#ifndef CEREBRUM_LUX_RESPONSE_ENGINE_H
#define CEREBRUM_LUX_RESPONSE_ENGINE_H

#include <string> // std::string için
#include <vector> // std::vector için
#include <map>    // std::map için
#include <random> // std::random_device, std::mt19937, std::uniform_int_distribution için

#include "../core/enums.h" // UserIntent, AbstractState, AIGoal için
#include "../core/utils.h" // For convert_wstring_to_string (if needed elsewhere)
#include "../data_models/dynamic_sequence.h" // DynamicSequence için ileri bildirim
#include "../brain/intent_analyzer.h" // IntentAnalyzer için ileri bildirim
#include "../planning_execution/goal_manager.h" // GoalManager için ileri bildirim
#include "ai_insights_engine.h" // AIInsightsEngine için ileri bildirim

// İleri bildirimler
struct DynamicSequence;
class IntentAnalyzer;
class GoalManager;
class AIInsightsEngine;

// Yanıt şablonlarını tutan yapı
struct ResponseTemplate {
    std::vector<std::string> responses; // std::wstring yerine std::string
    float trigger_threshold = 0.0f; // Keep this, it was in the original
};

// ResponseEngine sınıfı tanımı
class ResponseEngine {
public:
    ResponseEngine(IntentAnalyzer& analyzer_ref, GoalManager& goal_manager_ref, AIInsightsEngine& insights_engine_ref);

    // current_predicted_intent ve current_abstract_state'e göre dinamik yanıt üretir
    std::string generate_response(UserIntent current_intent, AbstractState current_abstract_state, AIGoal current_goal, const DynamicSequence& sequence) const; // std::wstring yerine std::string

private:
    IntentAnalyzer& analyzer;
    GoalManager& goal_manager;
    AIInsightsEngine& insights_engine;

    // Yanıt şablonları: Niyet ve Durum kombinasyonlarına göre
    std::map<UserIntent, std::map<AbstractState, ResponseTemplate>> response_templates;

    mutable std::mt19937 gen; // Rastgele sayı üreteci
    mutable std::random_device rd; // Rastgele sayı üreteci için seed
};

#endif // CEREBRUM_LUX_RESPONSE_ENGINE_H
