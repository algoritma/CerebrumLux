#include "meta_evolution_engine.h"
#include "../core/logger.h"
#include "../core/enums.h" // LogLevel için
#include "../sensors/atomic_signal.h" // sequenceManager için
#include <stdexcept> // std::runtime_error için
#include "../external/nlohmann/json.hpp" // JSON için

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
        // Adım 3: İçgörüler oluştur ve JSON olarak aktar.
        LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "MetaEvolutionEngine: AIInsightsEngine generate_insights çağrılıyor (JSON dönüşü bekleniyor).");
        std::string insights_json_str = insights_engine.generate_insights(current_sequence); // JSON string'ini al
        LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "MetaEvolutionEngine: generate_insights cagrisi TAMAMLANDI. Donen string boyutu: " << insights_json_str.length() << " (Line " << __LINE__ << ")"); // Log seviyesi TRACE'e düşürüldü
        LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "MetaEvolutionEngine: Donen string (ilk 500 char): '" << insights_json_str.substr(0, std::min(insights_json_str.length(), (size_t)500)) << "'"); // Log seviyesi TRACE'e düşürüldü
    
        std::vector<AIInsight> insights; // Yerel vektör
        if (!insights_json_str.empty()) {
            try {
                // Eğer string 21 karakter ve substr 71 hatası alıyorsak, string ya boş ya da bozuk demektir.
                // Güvenlik kontrolü ekleyelim.
                if (insights_json_str.length() < 2 && insights_json_str != "[]") { // JSON ayrıştırmadan önce boş veya çok kısa string kontrolü
                    throw std::runtime_error("AIInsightsEngine'den donen JSON string'i beklenenden kisa veya bozuk.");
                }
                nlohmann::json insights_json = nlohmann::json::parse(insights_json_str);
                insights = insights_json.get<std::vector<AIInsight>>(); // JSON'dan vektöre ayrıştır (Line 76)
                LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MetaEvolutionEngine: JSON ayrıştırıldı ve " << insights.size() << " adet içgörü yerel vektöre eklendi. (Line " << __LINE__ << ")");
            } catch (const nlohmann::json::exception& e) { // ✅ Düzeltme: nlohmann::json::json::exception yerine nlohmann::json::exception
                LOG_ERROR_CERR(CerebrumLux::LogLevel::ERR_CRITICAL, "MetaEvolutionEngine: JSON ayrıştırma hatası: " << e.what() << " (Line " << __LINE__ << ")");
            } catch (const std::exception& e) {
                LOG_ERROR_CERR(CerebrumLux::LogLevel::ERR_CRITICAL, "MetaEvolutionEngine: İçgörü JSON'dan dönüştürme hatası: " << e.what() << " (Line " << __LINE__ << ")");
            }
        } else {
            LOG_DEFAULT(CerebrumLux::LogLevel::WARNING, "MetaEvolutionEngine: AIInsightsEngine'den boş JSON string'i döndü. (Line " << __LINE__ << ")");
        }
        
        int codeDevCountReceived = 0; // Yerel CodeDev sayaç
        for (const auto& insight : insights) { // Artık ayrıştırılan 'insights' vektörü üzerinde döngü yapılıyor
            LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "MetaEvolutionEngine: Alınan icgoru (detay): ID=" << insight.id
                               << ", Type=" << static_cast<int>(insight.type)
                               << ", Context=" << insight.context
                               << ", FilePath=" << insight.code_file_path);
            LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "MetaEvolutionEngine: DIAGNOSTIC - Insight ID: " << insight.id << ", Actual Type: " << static_cast<int>(insight.type) << ", Expected CodeDev Type: " << static_cast<int>(CerebrumLux::InsightType::CodeDevelopmentSuggestion)); // Log seviyesi TRACE'e düşürüldü
            if (insight.type == CerebrumLux::InsightType::CodeDevelopmentSuggestion) { // CodeDev tespiti
                codeDevCountReceived++;
                LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MetaEvolutionEngine: !!! CodeDev İÇGÖRÜSÜ ALINDI (MetaEvolutionEngine'de Onaylandi). ID=" << insight.id << " (Line " << __LINE__ << ")");
            }
        }
        if (codeDevCountReceived > 0) {
            LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "MetaEvolutionEngine: Toplam CodeDev içgörüsü alındı: " << codeDevCountReceived);
        } else {
            LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "MetaEvolutionEngine: AIInsightsEngine'den hic CodeDev içgörüsü alınmadı.");
        }

        LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "MetaEvolutionEngine: LearningModule'e gonderilmeden once icgorulerin durumu (Toplam: " << insights.size() << "): (Line " << __LINE__ << ")"); // Log seviyesi TRACE'e düşürüldü
        for (const auto& insight : insights) { // İçgörüler üzerinde döngü
            LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "  - ID: " << insight.id << ", Type: " << static_cast<int>(insight.type) << ", Context: " << insight.context); // Log seviyesi TRACE'e düşürüldü
        }
        
        learning_module.process_ai_insights(insights); // Ayrıştırılan 'insights' vektörünü gönder
    } catch (const std::exception& e) {
        LOG_ERROR_CERR(CerebrumLux::LogLevel::ERR_CRITICAL, "MetaEvolutionEngine: AIInsightsEngine adiminda hata: " << e.what());
    }
    catch (...) {
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