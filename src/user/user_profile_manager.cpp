#include "user_profile_manager.h" // Kendi başlık dosyasını dahil et
#include "../core/logger.h"         // LOG makrosu için
#include "../core/utils.h"          // intent_to_string, abstract_state_to_string vb. için
#include "../data_models/dynamic_sequence.h" // DynamicSequence'in tam tanımı için
#include <fstream>                  // Dosya G/Ç için
#include <iostream>                 // Debug çıktıları için
#include <algorithm>                // std::sort, std::find_if için
#include <sstream>                  // std::stringstream için


// === UserProfileManager Implementasyonlari ===

// Kurucu
UserProfileManager::UserProfileManager() : history_max_size(50) {
    // Varsayılan tercihleri başlat (isteğe bağlı)
    set_preference("Theme", "Dark");
    set_preference("NotificationSounds", "Enabled");
    set_preference("DefaultBrowser", "Chrome");

    LOG_DEFAULT(LogLevel::INFO, "UserProfileManager: Başlatıldı. Varsayılan tercihler ayarlandı.\n");
}

// Tercihleri yönetme
void UserProfileManager::set_preference(const std::string& key, const std::string& value) {
    preferences[key].key = key;
    preferences[key].value = value;
    preferences[key].last_modified_us = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count();
    LOG_DEFAULT(LogLevel::DEBUG, "UserProfileManager: Tercih ayarlandı - " << key << ": " << value << "\n");
}

std::string UserProfileManager::get_preference(const std::string& key, const std::string& default_value) const {
    auto it = preferences.find(key);
    if (it != preferences.end()) {
        return it->second.value;
    }
    LOG_DEFAULT(LogLevel::WARNING, "UserProfileManager: '" << key << "' tercihi bulunamadı. Varsayılan değer (" << default_value << ") kullanılıyor.\n");
    return default_value;
}

bool UserProfileManager::has_preference(const std::string& key) const {
    return preferences.count(key);
}

// Uygulama kullanımını güncelleme
void UserProfileManager::update_app_usage(unsigned short app_hash) {
    long long current_time_us = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count();

    if (app_usage_data.count(app_hash)) {
        app_usage_data[app_hash].usage_count++;
        app_usage_data[app_hash].last_used_us = current_time_us;
    } else {
        app_usage_data[app_hash] = {app_hash, 1, current_time_us};
    }
    LOG_DEFAULT(LogLevel::DEBUG, "UserProfileManager: Uygulama kullanımı güncellendi - Hash: " << app_hash << ", Sayım: " << app_usage_data[app_hash].usage_count << "\n");
}

std::vector<FrequentAppInfo> UserProfileManager::get_frequent_apps(int top_n) const {
    std::vector<FrequentAppInfo> sorted_apps;
    for (const auto& pair : app_usage_data) {
        sorted_apps.push_back(pair.second);
    }
    // Kullanım sayısına göre azalan sırada sırala
    std::sort(sorted_apps.begin(), sorted_apps.end(), [](const FrequentAppInfo& a, const FrequentAppInfo& b) {
        return a.usage_count > b.usage_count;
    });

    if (top_n > 0 && sorted_apps.size() > top_n) {
        sorted_apps.resize(top_n);
    }
    return sorted_apps;
}

// Niyet ve durum geçmişini yönetme
void UserProfileManager::add_intent_history_entry(UserIntent intent, long long timestamp_us) {
    intent_history.push_back({intent, timestamp_us});
    trim_history(); // Geçmişi temizle
    LOG_DEFAULT(LogLevel::DEBUG, "UserProfileManager: Niyet geçmişine eklendi - Niyet: " << intent_to_string(intent) << "\n");
}

void UserProfileManager::add_state_history_entry(AbstractState state, long long timestamp_us) {
    state_history.push_back({state, timestamp_us});
    trim_history(); // Geçmişi temizle
    LOG_DEFAULT(LogLevel::DEBUG, "UserProfileManager: Durum geçmişine eklendi - Durum: " << abstract_state_to_string(state) << "\n");
}

// Kullanıcının bir eyleme verdiği açık geri bildirimi kaydeder - YENİ IMPLEMENTASYON
void UserProfileManager::add_explicit_action_feedback(UserIntent intent, AIAction action, bool approved) {
    personalized_action_feedback[intent][action].push_back(approved);
    // İsteğe bağlı: geçmiş boyutu kontrolü eklenebilir
    // Örneğin, belirli bir niyet-eylem çifti için sadece son N geri bildirimi tutmak.
    // if (personalized_action_feedback[intent][action].size() > feedback_history_size) {
    //     personalized_action_feedback[intent][action].erase(personalized_action_feedback[intent][action].begin());
    // }
    LOG_DEFAULT(LogLevel::DEBUG, "UserProfileManager: Açık eylem geri bildirimi kaydedildi - Niyet: " << intent_to_string(intent) << ", Eylem: " << static_cast<int>(action) << ", Onaylandı: " << (approved ? "Evet" : "Hayır") << "\n");
}


// Kişiselleştirilmiş geri bildirim döngüleri
float UserProfileManager::get_personalized_feedback_strength(UserIntent intent, AIAction action) const {
    auto it_intent = personalized_action_feedback.find(intent);
    if (it_intent != personalized_action_feedback.end()) {
        auto it_action = it_intent->second.find(action);
        if (it_action != it_intent->second.end() && !it_action->second.empty()) {
            float positive_count = 0.0f;
            for (bool approved : it_action->second) {
                if (approved) positive_count += 1.0f;
            }
            // Ortalama onay oranı döndür
            return positive_count / it_action->second.size();
        }
    }
    return 0.5f; // Varsayılan nötr (50%)
}

// `DynamicSequence`'ten gelen verilere göre profili günceller
void UserProfileManager::update_profile_from_sequence(const DynamicSequence& sequence) {
    LOG_DEFAULT(LogLevel::DEBUG, "UserProfileManager: Profil DynamicSequence'ten güncelleniyor...\n");
    // Uygulama kullanımını güncelle
    if (sequence.current_app_hash != 0) { // Geçerli bir uygulama hash'i varsa
        update_app_usage(sequence.current_app_hash);
    }

    // Diğer metrikleri kullanarak implicit tercihleri veya durumları güncelle (örnek)
    if (sequence.current_battery_percentage < 20 && !sequence.current_battery_charging && !has_preference("LowBatteryAction")) {
        set_preference("LowBatteryAction", "DimScreen"); // Düşük pil seviyesinde ekranı karartma tercihi
        LOG_DEFAULT(LogLevel::INFO, "UserProfileManager: Yeni tercih kaydedildi: LowBatteryAction=DimScreen\n");
    }
    // Daha fazla kişiselleştirme mantığı buraya eklenebilir.
}

// Geçmiş listelerini temizlemek için
void UserProfileManager::trim_history() {
    if (intent_history.size() > history_max_size) {
        intent_history.erase(intent_history.begin(), intent_history.begin() + (intent_history.size() - history_max_size));
    }
    if (state_history.size() > history_max_size) {
        state_history.erase(state_history.begin(), state_history.begin() + (state_history.size() - history_max_size));
    }
}

// Profil verilerini dosyaya kaydet
void UserProfileManager::save_profile(const std::string& filename) const {
    std::ofstream ofs(filename);
    if (!ofs.is_open()) {
        LOG_DEFAULT(LogLevel::ERR_CRITICAL, "Hata: Kullanıcı profili dosyası yazılamadı: " << filename << "\n");
        return;
    }

    // Tercihleri yaz
    ofs << "[PREFERENCES]\n";
    for (const auto& pair : preferences) {
        ofs << pair.second.key << "=" << pair.second.value << "|" << pair.second.last_modified_us << "\n";
    }

    // Uygulama kullanımını yaz
    ofs << "[APP_USAGE]\n";
    for (const auto& pair : app_usage_data) {
        ofs << pair.second.app_hash << "=" << pair.second.usage_count << "|" << pair.second.last_used_us << "\n";
    }
    
    // Niyet geçmişini yaz
    ofs << "[INTENT_HISTORY]\n";
    for (const auto& entry : intent_history) {
        ofs << static_cast<int>(entry.first) << "|" << entry.second << "\n";
    }

    // Durum geçmişini yaz
    ofs << "[STATE_HISTORY]\n";
    for (const auto& entry : state_history) {
        ofs << static_cast<int>(entry.first) << "|" << entry.second << "\n";
    }

    // Kişiselleştirilmiş eylem geri bildirimini yaz
    ofs << "[PERSONALIZED_ACTION_FEEDBACK]\n";
    for (const auto& intent_pair : personalized_action_feedback) {
        ofs << static_cast<int>(intent_pair.first) << ":\n";
        for (const auto& action_pair : intent_pair.second) {
            ofs << "  " << static_cast<int>(action_pair.first) << ":";
            for (bool approved : action_pair.second) {
                ofs << (approved ? "1" : "0");
            }
            ofs << "\n";
        }
    }

    ofs.close();
    LOG_DEFAULT(LogLevel::INFO, "UserProfileManager: Kullanıcı profili kaydedildi: " << filename << "\n");
}

// Profil verilerini dosyadan yükle
void UserProfileManager::load_profile(const std::string& filename) {
    std::ifstream ifs(filename);
    if (!ifs.is_open()) {
        LOG_DEFAULT(LogLevel::WARNING, "Uyarı: Kullanıcı profili dosyası bulunamadı, varsayılan profil kullanılıyor: " << filename << "\n");
        return;
    }

    preferences.clear();
    app_usage_data.clear();
    intent_history.clear();
    state_history.clear();
    personalized_action_feedback.clear();

    std::string line;
    std::string current_section;

    while (std::getline(ifs, line)) {
        if (line.empty()) continue;

        if (line.front() == '[' && line.back() == ']') {
            current_section = line;
            continue;
        }

        if (current_section == "[PREFERENCES]") {
            size_t eq_pos = line.find('=');
            size_t pipe_pos = line.find('|', eq_pos);
            if (eq_pos != std::string::npos && pipe_pos != std::string::npos) {
                std::string key = line.substr(0, eq_pos);
                std::string value = line.substr(eq_pos + 1, pipe_pos - (eq_pos + 1));
                long long timestamp = std::stoll(line.substr(pipe_pos + 1));
                preferences[key] = {key, value, timestamp};
            }
        } else if (current_section == "[APP_USAGE]") {
            size_t eq_pos = line.find('=');
            size_t pipe_pos = line.find('|', eq_pos);
            if (eq_pos != std::string::npos && pipe_pos != std::string::npos) {
                unsigned short app_hash = static_cast<unsigned short>(std::stoul(line.substr(0, eq_pos)));
                int usage_count = std::stoi(line.substr(eq_pos + 1, pipe_pos - (eq_pos + 1)));
                long long last_used_us = std::stoll(line.substr(pipe_pos + 1));
                app_usage_data[app_hash] = {app_hash, usage_count, last_used_us};
            }
        } else if (current_section == "[INTENT_HISTORY]") {
            size_t pipe_pos = line.find('|');
            if (pipe_pos != std::string::npos) {
                UserIntent intent = static_cast<UserIntent>(std::stoi(line.substr(0, pipe_pos)));
                long long timestamp = std::stoll(line.substr(pipe_pos + 1));
                intent_history.push_back({intent, timestamp});
            }
        } else if (current_section == "[STATE_HISTORY]") {
            size_t pipe_pos = line.find('|');
            if (pipe_pos != std::string::npos) {
                AbstractState state = static_cast<AbstractState>(std::stoi(line.substr(0, pipe_pos)));
                long long timestamp = std::stoll(line.substr(pipe_pos + 1));
                state_history.push_back({state, timestamp});
            }
        } else if (current_section == "[PERSONALIZED_ACTION_FEEDBACK]") {
            size_t colon_pos = line.find(':');
            if (colon_pos != std::string::npos) {
                std::string sub_line_start = line.substr(0, colon_pos);
                std::string sub_line_data = line.substr(colon_pos + 1);

                // Bu bölümün intent_id'si ile başlaması beklenir (örn: "1:")
                if (sub_line_start.find("  ") == std::string::npos) { // Niyet başlığı satırı (örn: "3:")
                    UserIntent intent_id = static_cast<UserIntent>(std::stoi(sub_line_start));
                    personalized_action_feedback[intent_id] = {}; // Yeni niyet için boş bir map başlat
                    // Bir sonraki satır veya satırlar bu niyete ait eylem geri bildirimlerini içerecektir.
                    // Okuma sırasında `current_intent` bilgisini takip etmek için geçici bir değişken kullanabiliriz
                    // Ancak bu karmaşıklığı artırır. Şimdilik bu varsayımla devam ediyoruz.
                    // Okuma sırasında bu intent_id'yi current_loading_intent olarak saklayabiliriz.
                } else { // Eylem geri bildirimi satırı (örn: "  1:10101")
                    // En son okunan niyete ait olduğunu varsayarak
                    if (!personalized_action_feedback.empty()) {
                        UserIntent last_read_intent = personalized_action_feedback.rbegin()->first;
                        int action_id = std::stoi(sub_line_start.substr(2)); // "  " kaldır
                        std::vector<bool> feedback_vector;
                        for (char c : sub_line_data) {
                            if (c == '1') feedback_vector.push_back(true);
                            else if (c == '0') feedback_vector.push_back(false);
                        }
                        personalized_action_feedback[last_read_intent][static_cast<AIAction>(action_id)] = feedback_vector;
                    }
                }
            }
        }
    }
    ifs.close();
    LOG_DEFAULT(LogLevel::INFO, "UserProfileManager: Kullanıcı profili yüklendi: " << filename << "\n");
}