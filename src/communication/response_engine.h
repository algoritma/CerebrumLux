#ifndef CEREBRUM_LUX_RESPONSE_ENGINE_H
#define CEREBRUM_LUX_RESPONSE_ENGINE_H

#include <vector>  // For std::vector
#include <string>  // For std::wstring
#include <map>     // For std::map
#include <random> // For random numbers
#include "../core/enums.h"               // Enumlar için
#include "../data_models/dynamic_sequence.h" // DynamicSequence için ileri bildirim
#include "../brain/intent_analyzer.h"       // IntentAnalyzer için ileri bildirim
#include "../planning_execution/goal_manager.h" // GoalManager için ileri bildirim
#include "ai_insights_engine.h"          // AIInsightsEngine için ileri bildirim
#include "../brain/autoencoder.h" // CryptofigAutoencoder::LATENT_DIM için


// İleri bildirimler
struct DynamicSequence;
class IntentAnalyzer;
class GoalManager;
class AIInsightsEngine;

// *** ResponseEngine: AI'ın kullaniciya metin tabanli yanitlar uretir ***
class ResponseEngine {
public:
    ResponseEngine(IntentAnalyzer& analyzer_ref, GoalManager& goal_manager_ref, AIInsightsEngine& insights_engine_ref); 

    std::wstring generate_response(UserIntent current_intent, AbstractState current_abstract_state, AIGoal current_goal, const DynamicSequence& sequence) const;

private:
    IntentAnalyzer& analyzer;
    GoalManager& goal_manager;
    AIInsightsEngine& insights_engine; 

    struct ResponseTemplate {
        std::vector<std::wstring> responses;
        float trigger_threshold = 0.0f; 
    };

    std::map<UserIntent, std::map<AbstractState, ResponseTemplate>> response_templates;

    mutable std::random_device rd;
    mutable std::mt19937 gen;
};


#endif // CEREBRUM_LUX_RESPONSE_ENGINE_H