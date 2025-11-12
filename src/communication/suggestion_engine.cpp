#include "suggestion_engine.h"
#include "../core/logger.h"
#include "../core/utils.h" // SafeRNG ve action_to_string için

namespace CerebrumLux {

SuggestionEngine::SuggestionEngine(IntentAnalyzer& analyzer_ref)
    : intent_analyzer(analyzer_ref),
      learning_rate(0.1f),
      discount_factor(0.9f),
      exploration_rate(0.2f)
{
    LOG_DEFAULT(LogLevel::INFO, "SuggestionEngine: Initialized.");
    // Q-tablosunu başlatma veya yükleme lojiği burada olabilir.
}

AIAction SuggestionEngine::suggest_action(UserIntent current_intent, AbstractState current_abstract_state, const DynamicSequence& sequence) {
    StateKey current_state = {current_intent, current_abstract_state};

    // Keşfetme (explore) veya sömürme (exploit)
    bool explore = (static_cast<float>(SafeRNG::getInstance().get_generator()()) / SafeRNG::getInstance().get_generator().max()) < exploration_rate;

    AIAction chosen_action = choose_action(current_state, explore);

    LOG_DEFAULT(LogLevel::DEBUG, "SuggestionEngine: State [Intent: " << intent_to_string(current_intent)
                                  << ", State: " << abstract_state_to_string(current_abstract_state)
                                  << "] için önerilen eylem: " << action_to_string(chosen_action));
    return chosen_action;
}

void SuggestionEngine::update_q_value(const StateKey& state, AIAction action, float reward) {
    float old_q_value = q_table[state][action];
    float max_future_q = get_max_q_value(state); // Gelecekteki durum için maksimum Q-değeri (bellman equation)

    // Q-öğrenme denklemi
    float new_q_value = old_q_value + learning_rate * (reward + discount_factor * max_future_q - old_q_value);

    q_table[state][action] = new_q_value;
    LOG_DEFAULT(LogLevel::DEBUG, "SuggestionEngine: Q-değeri güncellendi. State [Intent: " << intent_to_string(state.intent)
                                  << ", State: " << abstract_state_to_string(state.state)
                                  << "], Action: " << action_to_string(action) << ", Yeni Q-Değeri: " << new_q_value);
}

AIAction SuggestionEngine::choose_action(const StateKey& state, bool explore) const {
    if (explore) {
        // Rastgele eylem seç
        // Mevcut eylem setini dinamik olarak belirlemeliyiz
        // Şimdilik, tanımlı tüm AIAction'lar arasından rastgele seçelim.
        std::vector<AIAction> available_actions = {
            AIAction::None, AIAction::RespondToUser, AIAction::SuggestSelfImprovement,
            AIAction::AdjustLearningRate, AIAction::RequestMoreData, AIAction::QuarantineCapsule,
            AIAction::InitiateHandshake, AIAction::PerformWebSearch, AIAction::UpdateKnowledgeBase,
            AIAction::MonitorPerformance, AIAction::CalibrateSensors, AIAction::ExecutePlan
        };
        std::uniform_int_distribution<size_t> dist(0, available_actions.size() - 1);
        return available_actions[dist(SafeRNG::getInstance().get_generator())];
    } else {
        // En iyi eylemi seç (exploit)
        if (q_table.count(state) == 0) {
            return AIAction::None; // State henüz keşfedilmediyse varsayılan eylem
        }

        AIAction best_action = AIAction::None;
        float max_q_value = -std::numeric_limits<float>::infinity();

        for (const auto& pair : q_table.at(state)) {
            if (pair.second > max_q_value) {
                max_q_value = pair.second;
                best_action = pair.first;
            }
        }
        return best_action;
    }
}

float SuggestionEngine::get_max_q_value(const StateKey& state) const {
    if (q_table.count(state) == 0) {
        return 0.0f; // State henüz keşfedilmediyse sıfır döndür
    }

    float max_q_value = -std::numeric_limits<float>::infinity();
    for (const auto& pair : q_table.at(state)) {
        if (pair.second > max_q_value) {
            max_q_value = pair.second;
        }
    }
    return max_q_value;
}

} // namespace CerebrumLux