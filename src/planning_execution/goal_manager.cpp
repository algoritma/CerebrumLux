#include "goal_manager.h"
#include "../core/logger.h"
#include "../core/utils.h" // goal_to_string, intent_to_string için
#include "../core/enums.h" // AIAction, UserIntent, AbstractState için
#include "../external/nlohmann/json.hpp" // JSON için

namespace CerebrumLux {

GoalManager::GoalManager(AIInsightsEngine& insights_engine_ref)
    : insights_engine(insights_engine_ref),
      current_goal(AIGoal::OptimizeProductivity) // Varsayılan hedef
{
    LOG_DEFAULT(LogLevel::INFO, "GoalManager: Initialized. Current goal: " << goal_to_string(current_goal));
}

AIGoal GoalManager::get_current_goal() const {
    return current_goal;
}

void GoalManager::set_current_goal(AIGoal goal) {
    if (goal != current_goal) {
        current_goal = goal;
        LOG_DEFAULT(LogLevel::INFO, "GoalManager: Aktif hedef değişti: " << goal_to_string(current_goal));
    }
}

void GoalManager::evaluate_and_set_goal(const DynamicSequence& current_sequence) {
    // İçgörülere ve mevcut duruma göre hedef belirleme lojiği
    std::vector<AIInsight> insights;
    std::string insights_json_str; // ✅ DÜZELTME: insights_json_str try bloğunun dışında tanımlandı

    try {
        insights_json_str = insights_engine.generate_insights(current_sequence); // JSON string'ini al
        LOG_DEFAULT(LogLevel::DEBUG, "GoalManager: AIInsightsEngine'den JSON string alındı. Boyut: " << insights_json_str.length() << " İçerik: " << insights_json_str.substr(0, std::min((size_t)200, insights_json_str.length())));
    } catch (const std::exception& e) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "GoalManager: insights_engine.generate_insights çağrısında hata: " << e.what());
        return; // Hata durumunda döngüyü kes
    } catch (...) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "GoalManager: insights_engine.generate_insights çağrısında bilinmeyen hata.");
        return; // Hata durumunda döngüyü kes
    }

    if (!insights_json_str.empty()) {
        try {
            nlohmann::json insights_json = nlohmann::json::parse(insights_json_str);
            insights = insights_json.get<std::vector<AIInsight>>(); // JSON'dan vektöre ayrıştır
        } catch (const nlohmann::json::exception& e) {
            LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "GoalManager: Insights JSON ayrıştırma hatası: " << e.what());
            return; // Hata durumunda döngüyü kes
        } catch (const std::exception& e) {
            LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "GoalManager: Insights JSON'dan dönüştürme hatası: " << e.what());
            return; // Hata durumunda döngüyü kes
        }
    }
    LOG_DEFAULT(LogLevel::TRACE, "GoalManager: AIInsightsEngine'den " << insights.size() << " adet içgörü alındı (JSON dönüşüm sonrası).");

    // Basit bir örnek:
    for (const auto& insight : insights) {
        if (insight.type == InsightType::SecurityAlert && insight.urgency == UrgencyLevel::Critical) {
            set_current_goal(AIGoal::EnsureSecurity);
            return;
        }
        if (insight.type == InsightType::PerformanceAnomaly && insight.urgency == UrgencyLevel::High) {
            set_current_goal(AIGoal::OptimizeProductivity);
            return;
        }
        if (insight.type == InsightType::LearningOpportunity && insight.urgency == UrgencyLevel::High) {
            set_current_goal(AIGoal::MaximizeLearning);
            return;
        }
    }

    // Mevcut niyet analizi üzerinden de hedef belirle
    UserIntent analyzed_current_intent = UserIntent::Undefined; // Default değeri ayarla
    try {
        analyzed_current_intent = insights_engine.get_analyzer().analyze_intent(current_sequence);
        LOG_DEFAULT(LogLevel::DEBUG, "GoalManager: analyze_intent sonucu: " << intent_to_string(analyzed_current_intent));
    } catch (const std::exception& e) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "GoalManager: analyze_intent çağrısında hata: " << e.what());
        // Hata durumunda varsayılan niyetle devam edebiliriz veya döngüyü kesebiliriz.
    } catch (...) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "GoalManager: analyze_intent çağrısında bilinmeyen hata.");
    }

    // Mevcut niyet analizi üzerinden de hedef belirle
    if (analyzed_current_intent == UserIntent::Question || analyzed_current_intent == UserIntent::RequestInformation) {
        set_current_goal(AIGoal::ExploreNewKnowledge);
        return;
    }
    if (analyzed_current_intent == UserIntent::Command) {
        set_current_goal(AIGoal::OptimizeProductivity); // Komutları hızla yerine getirme hedefi
        return;
    }

    float reconstruction_error_val = 0.0f; // Default değeri ayarla
    try {
        reconstruction_error_val = insights_engine.calculate_autoencoder_reconstruction_error(current_sequence.statistical_features_vector);
        LOG_DEFAULT(LogLevel::DEBUG, "GoalManager: calculate_autoencoder_reconstruction_error sonucu: " << reconstruction_error_val);
    } catch (const std::exception& e) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "GoalManager: calculate_autoencoder_reconstruction_error çağrısında hata: " << e.what());
        // Hata durumunda varsayılan hata değeriyle devam edebiliriz.
    } catch (...) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "GoalManager: calculate_autoencoder_reconstruction_error çağrısında bilinmeyen hata.");
    }

    // Varsayılan durum
    if (current_sequence.current_network_active && current_sequence.network_activity_level == 0 && current_sequence.statistical_features_vector.size() == CryptofigAutoencoder::INPUT_DIM && reconstruction_error_val > 0.5f) {
        set_current_goal(AIGoal::MaximizeLearning); // Eğer autoencoder zorlanıyorsa öğrenmeye odaklan
    } else {
        set_current_goal(AIGoal::MaintainUserSatisfaction); // Genel olarak kullanıcıyı memnun etme
    }


    LOG_DEFAULT(LogLevel::DEBUG, "GoalManager: Hedef değerlendirildi. Aktif hedef: " << goal_to_string(current_goal));
}

void GoalManager::adjust_goals_based_on_feedback() {
    // Geri bildirime dayalı hedef ayarlama lojiği
    LOG_DEFAULT(LogLevel::DEBUG, "GoalManager: Geri bildirime dayalı hedefler ayarlandı.");
}

void GoalManager::evaluate_goals() {
    // Mevcut hedefleri değerlendirme lojiği
    LOG_DEFAULT(LogLevel::DEBUG, "GoalManager: Mevcut hedefler değerlendirildi.");
}

} // namespace CerebrumLux