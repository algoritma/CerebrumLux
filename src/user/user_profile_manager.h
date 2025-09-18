#ifndef CEREBRUM_LUX_USER_PROFILE_MANAGER_H
#define CEREBRUM_LUX_USER_PROFILE_MANAGER_H

#include <string>   // std::string için
#include <vector>   // std::vector için
#include <map>      // std::map için
#include <set>      // std::set için (tercihler için)
#include <chrono>   // Zaman bilgileri için

#include "../core/enums.h"         // Enumlar için
#include "../core/utils.h"         // hash_string vb. için
#include "../data_models/dynamic_sequence.h" // DynamicSequence'den metrikleri almak için

// İleri bildirimler (eğer başka sınıflardan referans alacaksa)
// class IntentLearner; // Örnek

// Kullanıcı tercihlerini ve geçmişini temsil eden yapı
struct UserPreference {
    std::string key;   // Örneğin: "DarkThemeEnabled", "NotificationSoundLevel"
    std::string value; // Örneğin: "true", "50"
    long long last_modified_us;
};

// Sık kullanılan uygulama bilgilerini tutan yapı
struct FrequentAppInfo {
    unsigned short app_hash;
    int usage_count;
    long long last_used_us;

    // std::map'te anahtar olarak kullanılabilmesi için karşılaştırma operatörü (isteğe bağlı)
    bool operator<(const FrequentAppInfo& other) const {
        return app_hash < other.app_hash;
    }
};

// *** UserProfileManager: Kullanıcı profilini ve tercihlerini yönetir ***
class UserProfileManager {
public:
    // Kurucu
    UserProfileManager();

    // Kullanıcı tercihlerini yükler ve kaydeder
    void load_profile(const std::string& filename);
    void save_profile(const std::string& filename) const;

    // Tercihleri yönetme
    void set_preference(const std::string& key, const std::string& value);
    std::string get_preference(const std::string& key, const std::string& default_value = "") const;
    bool has_preference(const std::string& key) const;

    // Uygulama kullanımını güncelleme
    void update_app_usage(unsigned short app_hash);
    std::vector<FrequentAppInfo> get_frequent_apps(int top_n = 5) const; // En sık kullanılan N uygulamayı döndürür

    // Niyet ve durum geçmişini yönetme (IntentLearner ile koordineli)
    void add_intent_history_entry(UserIntent intent, long long timestamp_us);
    void add_state_history_entry(AbstractState state, long long timestamp_us);

    // Kullanıcının bir eyleme verdiği açık geri bildirimi kaydeder - YENİ
    void add_explicit_action_feedback(UserIntent intent, AIAction action, bool approved);

    // Kişiselleştirilmiş geri bildirim döngüleri
    // Bu, AI'ın ResponseEngine veya Planner ile kişiselleştirilmiş yanıtlar/planlar üretmesine yardımcı olabilir.
    // Örneğin, belirli bir niyete veya duruma kullanıcının önceki tepkilerini değerlendirme.
    float get_personalized_feedback_strength(UserIntent intent, AIAction action) const;

    // `DynamicSequence`'ten gelen verilere göre profili günceller
    void update_profile_from_sequence(const DynamicSequence& sequence);

private:
    std::map<std::string, UserPreference> preferences;
    std::map<unsigned short, FrequentAppInfo> app_usage_data;
    std::vector<std::pair<UserIntent, long long>> intent_history; // Son N niyet
    std::vector<std::pair<AbstractState, long long>> state_history; // Son N durum

    // Pekiştirmeli öğrenme benzeri, kişiselleştirilmiş eylem/niyet başarı matrisi
    // Kullanıcının belirli durumlarda belirli eylemlere verdiği olumlu/olumsuz geri bildirimleri depolar.
    std::map<UserIntent, std::map<AIAction, std::vector<bool>>> personalized_action_feedback; // true=approved, false=rejected
    size_t history_max_size = 50; // Geçmiş tutulacak maksimum eleman sayısı

    // Yardımcı metotlar
    void trim_history(); // Geçmiş listelerini temizlemek için
};

#endif // CEREBRUM_LUX_USER_PROFILE_MANAGER_H