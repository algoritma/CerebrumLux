#ifndef CEREBRUM_LUX_SUGGESTION_ENGINE_H
#define CEREBRUM_LUX_SUGGESTION_ENGINE_H

#include <vector>  // For std::vector
#include <string>  // For std::wstring
#include <map>     // For std::map
#include <random>  // For random numbers
#include "../core/enums.h"               // Enumlar için
#include "../data_models/dynamic_sequence.h" // DynamicSequence için ileri bildirim
#include "../brain/intent_analyzer.h"       // IntentAnalyzer için ileri bildirim

// İleri bildirimler
struct DynamicSequence;
class IntentAnalyzer;

// *** SuggestionEngine: Tahmin edilen niyetlere dayanarak AI eylemleri onerir ***
class SuggestionEngine {
public:
    SuggestionEngine(IntentAnalyzer& analyzer_ref);

    AIAction suggest_action(UserIntent current_intent, const DynamicSequence& sequence);
    std::wstring action_to_string(AIAction action) const;

private:
    IntentAnalyzer& analyzer; 
    AIAction select_action_based_on_probability(UserIntent intent, const DynamicSequence& sequence); 
};

#endif // CEREBRUM_LUX_SUGGESTION_ENGINE_H