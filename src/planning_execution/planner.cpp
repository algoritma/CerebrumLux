#include "planner.h"
#include "../core/logger.h"
#include "../core/utils.h" // action_to_string, goal_to_string, intent_to_string için
#include "../core/enums.h" // AIAction için

namespace CerebrumLux {

Planner::Planner(IntentAnalyzer& analyzer_ref, SuggestionEngine& suggester_ref, GoalManager& goal_manager_ref, PredictionEngine& predictor_ref, AIInsightsEngine& insights_engine_ref)
    : intent_analyzer(analyzer_ref),
      suggestion_engine(suggester_ref),
      goal_manager(goal_manager_ref),
      prediction_engine(predictor_ref),
      insights_engine(insights_engine_ref)
{
    LOG_DEFAULT(LogLevel::INFO, "Planner: Initialized.");
}

std::vector<ActionPlanStep> Planner::create_action_plan(UserIntent current_intent, AbstractState current_abstract_state, AIGoal current_goal, const DynamicSequence& sequence) const {
    LOG_DEFAULT(LogLevel::DEBUG, "Planner: Niyet '" << intent_to_string(current_intent)
                                  << "', Durum '" << abstract_state_to_string(current_abstract_state)
                                  << "', Hedef '" << goal_to_string(current_goal) << "' için eylem planı oluşturuluyor.");

    std::vector<ActionPlanStep> plan;

    // İçgörülerden potansiyel eylemleri al
    std::vector<AIInsight> insights = insights_engine.generate_insights(sequence);
    for (const auto& insight : insights) {
        // İçgörüden gelen önerilen eylemleri plana ekle
        plan.push_back({insight.type == InsightType::LearningOpportunity ? AIAction::SuggestSelfImprovement : AIAction::None,
                        "İçgörüye dayalı öneri: " + insight.observation,
                        0.7f}); // Geçici güven değeri
    }

    // Hedefe dayalı bir plan oluştur
    std::vector<ActionPlanStep> goal_plan = generate_plan_for_goal(current_goal, current_intent, current_abstract_state, sequence);
    plan.insert(plan.end(), goal_plan.begin(), goal_plan.end());

    // Öneri motorundan ek eylemler al
    AIAction suggested_action = suggestion_engine.suggest_action(current_intent, current_abstract_state, sequence);
    if (suggested_action != AIAction::None) {
        plan.push_back({suggested_action, "Öneri motorundan gelen eylem: " + action_to_string(suggested_action), 0.8f});
    }

    LOG_DEFAULT(LogLevel::INFO, "Planner: Eylem planı oluşturuldu. Adım sayısı: " << plan.size());
    return plan;
}

std::vector<ActionPlanStep> Planner::generate_plan_for_goal(AIGoal goal, UserIntent intent, AbstractState state, const DynamicSequence& sequence) const {
    std::vector<ActionPlanStep> plan;

    if (goal == AIGoal::OptimizeProductivity) {
        plan.push_back({AIAction::RespondToUser, "Kullanıcıya hızlı yanıt ver", 0.9f});
        plan.push_back({AIAction::MonitorPerformance, "Klavye ve fare etkinliğini izle", 0.8f});
    } else if (goal == AIGoal::MaximizeLearning) {
        plan.push_back({AIAction::RequestMoreData, "Daha fazla veri topla", 0.9f});
        plan.push_back({AIAction::AdjustLearningRate, "Öğrenme oranını optimize et", 0.8f});
    } else if (goal == AIGoal::EnsureSecurity) {
        plan.push_back({AIAction::QuarantineCapsule, "Şüpheli kapsülleri karantinaya al", 1.0f});
        plan.push_back({AIAction::InitiateHandshake, "Güvenli bağlantı kur", 0.95f});
    } else if (goal == AIGoal::ExploreNewKnowledge) {
        plan.push_back({AIAction::PerformWebSearch, "İlgili konuları web'de araştır", 0.9f});
        plan.push_back({AIAction::UpdateKnowledgeBase, "Yeni bilgileri bilgi tabanına kaydet", 0.85f});
    } else {
        plan.push_back({AIAction::None, "Varsayılan eylem: Durumu izle", 0.5f});
    }
    return plan;
}

} // namespace CerebrumLux