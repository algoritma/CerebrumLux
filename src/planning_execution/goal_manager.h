#ifndef GOAL_MANAGER_H
#define GOAL_MANAGER_H

#include <string>
#include <vector>
#include "../core/enums.h" // AIGoal, UserIntent için
#include "../data_models/dynamic_sequence.h" // DynamicSequence için
#include "../communication/ai_insights_engine.h" // AIInsightsEngine için

namespace CerebrumLux { // GoalManager sınıfı bu namespace içine alınacak

class GoalManager {
public:
    GoalManager(AIInsightsEngine& insights_engine_ref);

    virtual AIGoal get_current_goal() const;
    void set_current_goal(AIGoal goal);
    void evaluate_and_set_goal(const DynamicSequence& current_sequence);

    void adjust_goals_based_on_feedback(); // Meta-yönetimden çağrılabilir
    void evaluate_goals(); // Mevcut hedefleri değerlendir

private:
    AIInsightsEngine& insights_engine;
    AIGoal current_goal; // Mevcut aktif hedef

    // Hedef öncelikleri, koşulları vb. burada yönetilebilir
    // std::map<AIGoal, float> goal_priorities;
    // std::map<AIGoal, std::function<bool(const DynamicSequence&)>> goal_conditions;
};

} // namespace CerebrumLux

#endif // GOAL_MANAGER_H
