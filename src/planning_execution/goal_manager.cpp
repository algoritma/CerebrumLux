#include "goal_manager.h" 
#include "../core/logger.h"
#include "../core/utils.h"
#include "../data_models/dynamic_sequence.h"
#include "../communication/ai_insights_engine.h"
#include "../brain/intent_analyzer.h"
#include "../brain/autoencoder.h"
#include <vector>
#include <algorithm>
#include <iostream>
#include <sstream>

// === GoalManager Implementasyonlari ===
GoalManager::GoalManager(AIInsightsEngine& insights_engine_ref) : current_goal(AIGoal::OptimizeProductivity), insights_engine(insights_engine_ref) {}

AIGoal GoalManager::get_current_goal() const {
    return current_goal;
}

void GoalManager::set_current_goal(AIGoal new_goal) {
    if (current_goal != new_goal) {
        current_goal = new_goal;
        LOG(LogLevel::INFO, std::wcout, L"[AI-Hedef] Yeni hedef ayarlandi: " << goal_to_string(new_goal));
    }
}

// Dinamik hedef belirleme fonksiyonu
void GoalManager::evaluate_and_set_goal(const DynamicSequence& current_sequence) {
    LOG(LogLevel::DEBUG, std::wcout, L"GoalManager::evaluate_and_set_goal: Dinamik hedef belirleme basladi.\n");
    std::vector<AIInsight> insights = insights_engine.generate_insights(current_sequence);

    // En yüksek aciliyetli içgörüye göre hedef belirle
    float max_urgency = 0.0f;
    AIAction critical_action = AIAction::None;

    for (const auto& insight : insights) {
        if (insight.urgency > max_urgency) {
            max_urgency = insight.urgency;
            critical_action = insight.suggested_action;
        }
    }

    UserIntent analyzed_current_intent = insights_engine.get_analyzer().analyze_intent(current_sequence);

    if (max_urgency > 0.7f && critical_action == AIAction::SuggestSelfImprovement) {
        set_current_goal(AIGoal::SelfImprovement);
    } else if (current_sequence.current_battery_percentage < 20 && !current_sequence.current_battery_charging) {
        set_current_goal(AIGoal::MaximizeBatteryLife);
    } else if (current_sequence.current_network_active && current_sequence.network_activity_level == 0 && current_sequence.statistical_features_vector.size() == CryptofigAutoencoder::INPUT_DIM && insights_engine.calculate_autoencoder_reconstruction_error(current_sequence.statistical_features_vector) > 0.5f) {
        set_current_goal(AIGoal::ReduceDistractions); 
    } else if (analyzed_current_intent == UserIntent::IdleThinking && current_sequence.mouse_movement_intensity / 500.0f > 0.2f && current_sequence.network_activity_level / 15000.0f > 0.2f) { 
        set_current_goal(AIGoal::ReduceDistractions);
    }
    else {
        set_current_goal(AIGoal::OptimizeProductivity);
    }
    LOG(LogLevel::DEBUG, std::wcout, L"GoalManager::evaluate_and_set_goal: Dinamik hedef belirleme bitti. Mevcut hedef: " << static_cast<int>(current_goal) << L"\n");
}