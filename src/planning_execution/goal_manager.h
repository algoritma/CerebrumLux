#ifndef CEREBRUM_LUX_GOAL_MANAGER_H
#define CEREBRUM_LUX_GOAL_MANAGER_H

#include <string> // For wstring
#include "../core/enums.h"               // Enumlar için
#include "../core/utils.h"               // LOG için
#include "../data_models/dynamic_sequence.h" // DynamicSequence için ileri bildirim
#include "../communication/ai_insights_engine.h" // AIInsightsEngine için ileri bildirim
#include "../brain/intent_analyzer.h" // IntentAnalyzer için ileri bildirim

// İleri bildirimler
class AIInsightsEngine;
struct DynamicSequence;
class IntentAnalyzer;

// *** GoalManager: AI'ın hedeflerini yonetir ***
class GoalManager {
public:
    GoalManager(AIInsightsEngine& insights_engine_ref); 
    virtual AIGoal get_current_goal() const; // Eklendi: virtual
    void set_current_goal(AIGoal goal);
    void evaluate_and_set_goal(const DynamicSequence& current_sequence); // Dinamik hedef belirleme
private:
    AIGoal current_goal = AIGoal::OptimizeProductivity; 
    AIInsightsEngine& insights_engine; // AIInsightsEngine referansı
    // IntentAnalyzer'a doğrudan erişim insights_engine üzerinden yapılmalı, burada tutulmasına gerek yok.
};

#endif // CEREBRUM_LUX_GOAL_MANAGER_H