#ifndef PLANNER_H
#define PLANNER_H

#include <string>
#include <vector>
#include "../core/enums.h" // UserIntent, AbstractState, AIGoal, AIAction için
#include "../data_models/dynamic_sequence.h" // DynamicSequence için
#include "../brain/intent_analyzer.h" // IntentAnalyzer için
#include "../communication/suggestion_engine.h" // SuggestionEngine için
#include "../planning_execution/goal_manager.h" // GoalManager için
#include "../communication/ai_insights_engine.h" // AIInsightsEngine, AIInsight için

namespace CerebrumLux { // Planner sınıfı ve ActionPlanStep struct'ı bu namespace içine alınacak

// Bir eylem planındaki tek bir adımı temsil eder
struct ActionPlanStep {
    AIAction action;
    std::string description;
    float expected_outcome_confidence; // Bu adımın beklenen başarı güveni

    // JSON serileştirme desteği için
    // NLOHMANN_DEFINE_TYPE_INTRUSIVE(ActionPlanStep, action, description, expected_outcome_confidence)
};

class Planner {
public:
    Planner(IntentAnalyzer& analyzer, SuggestionEngine& suggester, GoalManager& goal_manager, PredictionEngine& predictor, AIInsightsEngine& insights_engine);

    virtual std::vector<ActionPlanStep> create_action_plan(UserIntent current_intent, AbstractState current_abstract_state, AIGoal current_goal, const DynamicSequence& sequence) const;

private:
    IntentAnalyzer& intent_analyzer;
    SuggestionEngine& suggestion_engine;
    GoalManager& goal_manager;
    PredictionEngine& prediction_engine;
    AIInsightsEngine& insights_engine;

    // Yardımcı fonksiyonlar
    std::vector<ActionPlanStep> generate_plan_for_goal(AIGoal goal, UserIntent intent, AbstractState state, const DynamicSequence& sequence) const;
};

} // namespace CerebrumLux

#endif // PLANNER_H
