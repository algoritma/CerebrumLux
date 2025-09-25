#ifndef SUGGESTION_ENGINE_H
#define SUGGESTION_ENGINE_H

#include <string>
#include <vector>
#include <map>
#include <algorithm> // std::max için
#include "../core/enums.h" // UserIntent, AbstractState, AIAction için
#include "../brain/intent_analyzer.h" // IntentAnalyzer için
#include "../data_models/dynamic_sequence.h" // DynamicSequence için

namespace CerebrumLux { // SuggestionEngine ve StateKey struct'ı bu namespace içine alınacak

// Reinforcement Learning için durum anahtarı
struct StateKey {
    UserIntent intent;
    AbstractState state;

    bool operator<(const StateKey& other) const {
        if (intent != other.intent) {
            return intent < other.intent;
        }
        return state < other.state;
    }
};

class SuggestionEngine {
public:
    SuggestionEngine(IntentAnalyzer& analyzer);

    virtual AIAction suggest_action(UserIntent current_intent, AbstractState current_abstract_state, const DynamicSequence& sequence);
    virtual void update_q_value(const StateKey& state, AIAction action, float reward);

private:
    IntentAnalyzer& intent_analyzer; // Analizci referansı
    std::map<StateKey, std::map<AIAction, float>> q_table; // Q-tablosu
    float learning_rate; // Alpha
    float discount_factor; // Gamma
    float exploration_rate; // Epsilon

    // Yardımcı fonksiyonlar
    AIAction choose_action(const StateKey& state, bool explore) const;
    float get_max_q_value(const StateKey& state) const;
};

} // namespace CerebrumLux

#endif // SUGGESTION_ENGINE_H
