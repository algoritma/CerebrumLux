#include "meta_evolution_engine.h"
#include "../core/logger.h"
#include "../core/enums.h" // LogLevel için
#include "../sensors/atomic_signal.h" // sequenceManager için
#include <stdexcept> // std::runtime_error için

namespace CerebrumLux {

MetaEvolutionEngine::MetaEvolutionEngine(
    IntentAnalyzer& analyzer_ref,
    IntentLearner& learner_ref,
    PredictionEngine& predictor_ref,
    GoalManager& goal_manager_ref,
    CryptofigProcessor& cryptofig_processor_ref,
    AIInsightsEngine& insights_engine_ref,
    LearningModule& learning_module_ref
) : 
    analyzer(analyzer_ref),
    learner(learner_ref),
    predictor(predictor_ref),
    goal_manager(goal_manager_ref),
    cryptofig_processor(cryptofig_processor_ref),
    insights_engine(insights_engine_ref),
    learning_module(learning_module_ref)
{
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "MetaEvolutionEngine: Initialized.");
}

void MetaEvolutionEngine::run_meta_evolution_cycle(const DynamicSequence& current_sequence) {
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MetaEvolutionEngine: Running meta-evolution cycle...");

    try {
        // Adım 1: Mevcut durumu analiz et ve hedefleri değerlendir.
        LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "MetaEvolutionEngine: GoalManager evaluate_and_set_goal çağrılıyor.");
        goal_manager.evaluate_and_set_goal(current_sequence);
        LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "MetaEvolutionEngine: GoalManager evaluate_and_set_goal tamamlandı. Güncel hedef: " << CerebrumLux::goal_to_string(goal_manager.get_current_goal()));
    } catch (const std::exception& e) {
        LOG_ERROR_CERR(CerebrumLux::LogLevel::ERR_CRITICAL, "MetaEvolutionEngine: GoalManager adiminda hata: " << e.what());
        return;
    } catch (...) {
        LOG_ERROR_CERR(CerebrumLux::LogLevel::ERR_CRITICAL, "MetaEvolutionEngine: GoalManager adiminda bilinmeyen hata.");
        return;
    }

    try {
        // Adım 2: Öngörüde bulun.
        LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "MetaEvolutionEngine: PredictionEngine predict_next_intent çağrılıyor.");
        CerebrumLux::UserIntent predicted_intent = predictor.predict_next_intent(CerebrumLux::UserIntent::Undefined, current_sequence);
        LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MetaEvolutionEngine: Tahmin edilen niyet: " << CerebrumLux::intent_to_string(predicted_intent));
    } catch (const std::exception& e) {
        LOG_ERROR_CERR(CerebrumLux::LogLevel::ERR_CRITICAL, "MetaEvolutionEngine: PredictionEngine adiminda hata: " << e.what());
        return;
    } catch (...) {
        LOG_ERROR_CERR(CerebrumLux::LogLevel::ERR_CRITICAL, "MetaEvolutionEngine: PredictionEngine adiminda bilinmeyen hata.");
        return;
    }

    try {
        // Adım 3: İçgörüler oluştur.
        LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "MetaEvolutionEngine: AIInsightsEngine generate_insights çağrılıyor.");
        const std::vector<AIInsight>& insights = insights_engine.generate_insights(current_sequence); // DEĞİŞTİRİLDİ: const referans al
        LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MetaEvolutionEngine: " << insights.size() << " adet içgörü üretildi.");
        
        // YENİ TEŞHİS LOGU: LearningModule'a aktarılmadan önce insights vektörünün içeriğini göster
        LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MetaEvolutionEngine: LearningModule'a AKTARILACAK İçgörüler (Toplam: " << insights.size() << "):");
        for (const auto& insight : insights) {
            LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MetaEvolutionEngine: LearningModule'a aktarılan İçgörü (Özet): ID=" << insight.id
                                      << ", Type=" << static_cast<int>(insight.type) << ", Context=" << insight.context);
        }
        
        learning_module.process_ai_insights(insights); // DEĞİŞTİRİLDİ: Doğrudan referansı ilet
    } catch (const std::exception& e) {
        LOG_ERROR_CERR(CerebrumLux::LogLevel::ERR_CRITICAL, "MetaEvolutionEngine: AIInsightsEngine adiminda hata: " << e.what());
        return;
    } catch (...) {
        LOG_ERROR_CERR(CerebrumLux::LogLevel::ERR_CRITICAL, "MetaEvolutionEngine: AIInsightsEngine adiminda bilinmeyen hata.");
        return;
    }

    try {
        // Adım 4: Kriptofigleri işle ve öğrenme için kullan.
        LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "MetaEvolutionEngine: CryptofigProcessor process_sequence çağrılıyor.");
        
        // current_sequence const olduğu için, kopyasını oluşturup mutable hale getiriyoruz.
        // Ayrıca, boş vector hatasını önlemek için kontrol ekliyoruz.
        if (current_sequence.statistical_features_vector.empty() || current_sequence.statistical_features_vector.size() != CryptofigAutoencoder::INPUT_DIM) {
            LOG_DEFAULT(CerebrumLux::LogLevel::WARNING, "MetaEvolutionEngine: current_sequence.statistical_features_vector boş veya boyutu CryptofigAutoencoder::INPUT_DIM ile uyuşmuyor. CryptofigProcessor işlemi atlanıyor.");
        } else {
            DynamicSequence mutable_sequence_copy = current_sequence; // Kopyasını alıyoruz
            cryptofig_processor.process_sequence(mutable_sequence_copy, 0.01f); // learning_rate_ae dummy olarak verildi
            LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "MetaEvolutionEngine: CryptofigProcessor process_sequence tamamlandı.");
        }
    } catch (const std::exception& e) {
        LOG_ERROR_CERR(CerebrumLux::LogLevel::ERR_CRITICAL, "MetaEvolutionEngine: CryptofigProcessor adiminda hata: " << e.what());
        return;
    } catch (...) {
        LOG_ERROR_CERR(CerebrumLux::LogLevel::ERR_CRITICAL, "MetaEvolutionEngine: CryptofigProcessor adiminda bilinmeyen hata.");
        return;
    }
    
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MetaEvolutionEngine: Meta-evolution cycle tamamlandı.");
}

} // namespace CerebrumLux