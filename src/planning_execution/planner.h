#ifndef CEREBRUM_LUX_PLANNER_H
#define CEREBRUM_LUX_PLANNER_H

#include <vector>  // For std::vector
#include <string>  // For std::string (was std::wstring)
#include "../core/enums.h"         // Enumlar için
#include "../core/utils.h"         // For convert_wstring_to_string (if needed elsewhere)
#include "../data_models/dynamic_sequence.h" // DynamicSequence için ileri bildirim
#include "../brain/intent_analyzer.h" // IntentAnalyzer için ileri bildirim
#include "../brain/prediction_engine.h" // PredictionEngine için ileri bildirim
#include "../communication/suggestion_engine.h" // SuggestionEngine için ileri bildirim
#include "../communication/ai_insights_engine.h" // AIInsightsEngine için ileri bildirim
#include "../planning_execution/goal_manager.h" // GoalManager için ileri bildirim

// İleri bildirimler
struct DynamicSequence;
class IntentAnalyzer;
class SuggestionEngine;
class GoalManager;
class PredictionEngine;
class AIInsightsEngine;

// === ActionPlanStep: Eylem planı adımı ===
struct ActionPlanStep {
    AIAction action;
    UserIntent triggered_by_intent;
    std::string description; // std::wstring yerine std::string

    ActionPlanStep(AIAction act, UserIntent intent, const std::string& desc); // const std::wstring& yerine const std::string&
};

// === Planner: AI'ın eylem planlarını oluşturur ve yürütür ===
class Planner {
public:
    Planner(IntentAnalyzer& analyzer_ref, SuggestionEngine& suggester_ref,
            GoalManager& goal_manager_ref, PredictionEngine& predictor_ref,
            AIInsightsEngine& insights_engine_ref);

    // Mevcut duruma göre bir eylem planı oluşturur
    std::vector<ActionPlanStep> create_action_plan(UserIntent current_intent, AbstractState current_abstract_state, AIGoal current_goal, const DynamicSequence& sequence) const;

    // Oluşturulan eylem planını yürütür (simüle eder)
    void execute_plan(const std::vector<ActionPlanStep>& plan);

private:
    IntentAnalyzer& analyzer;
    SuggestionEngine& suggester;
    GoalManager& goal_manager;
    PredictionEngine& predictor;
    AIInsightsEngine& insights_engine;

    // Yardımcı planlama fonksiyonları
    std::vector<ActionPlanStep> _plan_for_productivity(UserIntent current_intent, AbstractState current_abstract_state, const DynamicSequence& sequence) const;
    std::vector<ActionPlanStep> _plan_for_battery_life(UserIntent current_intent, AbstractState current_abstract_state, const DynamicSequence& sequence) const;
};

#endif // CEREBRUM_LUX_PLANNER_H