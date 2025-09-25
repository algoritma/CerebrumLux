#ifndef USER_PROFILE_MANAGER_H
#define USER_PROFILE_MANAGER_H

#include <string>
#include <vector>
#include <map>
#include <chrono> // std::chrono::system_clock için
#include <deque> // Tarihçe için
#include "../core/enums.h" // UserIntent, AbstractState, AIAction için

namespace CerebrumLux { // UserProfileManager sınıfı bu namespace içine alınacak

class UserProfileManager {
public:
    UserProfileManager();

    // Kullanıcı tercihlerini ve alışkanlıklarını kaydet
    void set_user_preference(const std::string& key, const std::string& value);
    std::string get_user_preference(const std::string& key) const;

    // Niyet ve durum tarihçesini güncelle
    void add_intent_history_entry(UserIntent intent, long long timestamp_us);
    void add_state_history_entry(AbstractState state, long long timestamp_us);

    // Açık geri bildirim kaydet (örneğin, bir öneriyi onaylama/reddetme)
    void add_explicit_action_feedback(UserIntent intent, AIAction action, bool approved);

    // Kişiselleştirilmiş geri bildirim gücünü hesapla
    float get_personalized_feedback_strength(UserIntent intent, AIAction action) const;

    // Tarihçe limitini belirle
    void set_history_limit(size_t limit);

private:
    std::map<std::string, std::string> user_preferences;

    // Niyet ve durum geçmişi (deque ile sınırlı boyut)
    size_t history_limit;
    std::deque<std::pair<UserIntent, long long>> intent_history; // Son N niyet
    std::deque<std::pair<AbstractState, long long>> state_history; // Son N durum

    // Kullanıcının eylemlere verdiği açık geri bildirim
    std::map<UserIntent, std::map<AIAction, std::vector<bool>>> personalized_action_feedback; // true=approved, false=rejected
};

} // namespace CerebrumLux

#endif // USER_PROFILE_MANAGER_H
