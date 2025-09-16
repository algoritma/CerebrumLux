#ifndef CEREBRUM_LUX_SUGGESTION_ENGINE_H
#define CEREBRUM_LUX_SUGGESTION_ENGINE_H

#include <string> // std::string için
#include <vector> // std::vector için
#include "../core/enums.h"         // Enumlar için
#include "../core/utils.h"         // For convert_wstring_to_string (if needed elsewhere)
#include "../data_models/dynamic_sequence.h" // DynamicSequence için ileri bildirim
#include "../brain/intent_analyzer.h" // IntentAnalyzer için ileri bildirim

// İleri bildirimler
struct DynamicSequence;
class IntentAnalyzer;

// *** SuggestionEngine: AI'ın kullaniciya eylem önerileri sunar ***
class SuggestionEngine {
public:
    SuggestionEngine(IntentAnalyzer& analyzer_ref);

    // Belirli bir niyet için en uygun eylemi önerir
    AIAction suggest_action(UserIntent current_intent, const DynamicSequence& sequence);

    // AIAction enum değerini string'e çevirir
    std::string action_to_string(AIAction action) const; // std::wstring yerine std::string

private:
    IntentAnalyzer& analyzer;

    // Olasılık tabanlı eylem seçimi
    AIAction select_action_based_on_probability(UserIntent intent, const DynamicSequence& sequence);
};

#endif // CEREBRUM_LUX_SUGGESTION_ENGINE_H
