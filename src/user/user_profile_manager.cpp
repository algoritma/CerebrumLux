#include "user_profile_manager.h"
#include "../core/logger.h"
#include "../core/utils.h" // intent_to_string, abstract_state_to_string, action_to_string için
#include <algorithm> // std::min için
#include <numeric> // Ortalama için

namespace CerebrumLux {

UserProfileManager::UserProfileManager()
    : history_limit(100) // Varsayılan tarihçe limiti
{
    LOG_DEFAULT(LogLevel::INFO, "UserProfileManager: Initialized.");
}

void UserProfileManager::set_user_preference(const std::string& key, const std::string& value) {
    user_preferences[key] = value;
    LOG_DEFAULT(LogLevel::DEBUG, "UserProfileManager: Kullanıcı tercihi ayarlandı: " << key << " = " << value);
}

std::string UserProfileManager::get_user_preference(const std::string& key) const {
    auto it = user_preferences.find(key);
    if (it != user_preferences.end()) {
        return it->second;
    }
    return ""; // Bulunamazsa boş döndür
}

void UserProfileManager::add_intent_history_entry(UserIntent intent, long long timestamp_us) {
    intent_history.push_back({intent, timestamp_us});
    if (intent_history.size() > history_limit) {
        intent_history.pop_front();
    }
    LOG_DEFAULT(LogLevel::TRACE, "UserProfileManager: Niyet tarihçesine eklendi: " << CerebrumLux::to_string(intent));
}

void UserProfileManager::add_state_history_entry(AbstractState state, long long timestamp_us) {
    state_history.push_back({state, timestamp_us});
    if (state_history.size() > history_limit) {
        state_history.pop_front();
    }
    LOG_DEFAULT(LogLevel::TRACE, "UserProfileManager: Durum tarihçesine eklendi: " << abstract_state_to_string(state));
}

void UserProfileManager::add_explicit_action_feedback(UserIntent intent, AIAction action, bool approved) {
    personalized_action_feedback[intent][action].push_back(approved);
    // Geri bildirim geçmişini sınırlayabiliriz.
    LOG_DEFAULT(LogLevel::DEBUG, "UserProfileManager: Açık eylem geri bildirimi: Niyet '" << CerebrumLux::to_string(intent)
                                  << "', Eylem '" << CerebrumLux::to_string(action) << "', Onaylandı: " << (approved ? "Evet" : "Hayır"));
}

float UserProfileManager::get_personalized_feedback_strength(UserIntent intent, AIAction action) const {
    auto intent_it = personalized_action_feedback.find(intent);
    if (intent_it == personalized_action_feedback.end()) {
        return 0.5f; // Varsayılan nötr değer
    }

    auto action_it = intent_it->second.find(action);
    if (action_it == intent_it->second.end()) {
        return 0.5f; // Varsayılan nötr değer
    }

    const std::vector<bool>& feedbacks = action_it->second;
    if (feedbacks.empty()) {
        return 0.5f;
    }

    float positive_count = 0;
    for (bool approved : feedbacks) {
        if (approved) {
            positive_count++;
        }
    }

    return positive_count / static_cast<float>(feedbacks.size());
}

void UserProfileManager::set_history_limit(size_t limit) {
    history_limit = limit;
    // Mevcut tarihçeleri de kırpabiliriz.
    if (intent_history.size() > history_limit) {
        intent_history.resize(history_limit);
    }
    if (state_history.size() > history_limit) {
        state_history.resize(history_limit);
    }
    LOG_DEFAULT(LogLevel::INFO, "UserProfileManager: Tarihçe limiti " << history_limit << " olarak ayarlandı.");
}

} // namespace CerebrumLux