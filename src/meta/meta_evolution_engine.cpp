#include "meta_evolution_engine.h"
#include "../core/logger.h"
#include "../core/enums.h" // LogLevel, UrgencyLevel, AIAction için
#include "../sensors/atomic_signal.h" // sequenceManager için
#include <stdexcept> // std::runtime_error için
#include "../external/nlohmann/json.hpp" // JSON için
#include "../core/utils.h" // SafeRNG için (rastgele seçim için)
#include "../crypto/CryptoManager.h" // CerebrumLux::Crypto::CryptoManager ve vec_to_str için

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

    // current_state_embedding'i döngü başında alıyoruz, çünkü birçok adımda kullanılacak.
    std::vector<float> current_state_embedding = current_sequence.latent_cryptofig_vector;
    if (current_state_embedding.empty() || current_state_embedding.size() != CerebrumLux::CryptofigAutoencoder::LATENT_DIM) {
        LOG_ERROR_CERR(CerebrumLux::LogLevel::ERR_CRITICAL, "MetaEvolutionEngine: Current sequence embedding boş veya boyutu uyuşmuyor. Meta-evolution cycle atlanıyor.");
        return;
    }


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

    // Adım 2: Öngörüde bulun. (Şimdilik eylem seçimi için doğrudan Q-table'a bakacağız)
    LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "MetaEvolutionEngine: PredictionEngine predict_next_intent çağrılıyor (şimdilik doğrudan kullanılmuyor).");
    CerebrumLux::UserIntent predicted_intent = predictor.predict_next_intent(CerebrumLux::UserIntent::Undefined, current_sequence);
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MetaEvolutionEngine: Tahmin edilen niyet: " << CerebrumLux::intent_to_string(predicted_intent));


    std::vector<AIInsight> insights; // Yerel içgörü vektörü
    try {
        // Adım 3: İçgörüler oluştur ve JSON olarak aktar.
        LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "MetaEvolutionEngine: AIInsightsEngine generate_insights çağrılıyor (JSON dönüşü bekleniyor).");
        std::string insights_json_str = insights_engine.generate_insights(current_sequence); // JSON string'ini al
        LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "MetaEvolutionEngine: generate_insights cagrisi TAMAMLANDI. Donen string boyutu: " << insights_json_str.length());
        LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "MetaEvolutionEngine: Donen string (ilk 500 char): '" << insights_json_str.substr(0, std::min(insights_json_str.length(), (size_t)500)) << "'");
    
        if (!insights_json_str.empty()) {
            try {
                if (insights_json_str.length() < 2 && insights_json_str != "[]") { 
                    throw std::runtime_error("AIInsightsEngine'den donen JSON string'i beklenenden kisa veya bozuk.");
                }
                nlohmann::json insights_json = nlohmann::json::parse(insights_json_str);
                insights = insights_json.get<std::vector<AIInsight>>();
                LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MetaEvolutionEngine: JSON ayrıştırıldı ve " << insights.size() << " adet içgörü yerel vektöre eklendi.");
            } catch (const nlohmann::json::exception& e) {
                LOG_ERROR_CERR(CerebrumLux::LogLevel::ERR_CRITICAL, "MetaEvolutionEngine: JSON ayrıştırma hatası: " << e.what());
            } catch (const std::exception& e) {
                LOG_ERROR_CERR(CerebrumLux::LogLevel::ERR_CRITICAL, "MetaEvolutionEngine: İçgörü JSON'dan dönüştürme hatası: " << e.what());
            }
        } else {
            LOG_DEFAULT(CerebrumLux::LogLevel::WARNING, "MetaEvolutionEngine: AIInsightsEngine'den boş JSON string'i döndü.");
        }
        
        int codeDevCountReceived = 0;
        for (const auto& insight : insights) {
            LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "MetaEvolutionEngine: Alınan icgoru (detay): ID=" << insight.id
                               << ", Type=" << static_cast<int>(insight.type)
                               << ", Context=" << insight.context
                               << ", FilePath=" << insight.code_file_path);
            LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "MetaEvolutionEngine: DIAGNOSTIC - Insight ID: " << insight.id << ", Actual Type: " << static_cast<int>(insight.type) << ", Expected CodeDev Type: " << static_cast<int>(CerebrumLux::InsightType::CodeDevelopmentSuggestion));
            if (insight.type == CerebrumLux::InsightType::CodeDevelopmentSuggestion) {
                codeDevCountReceived++;
                LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MetaEvolutionEngine: !!! CodeDev İÇGÖRÜSÜ ALINDI (MetaEvolutionEngine'de Onaylandi). ID=" << insight.id);
            }
        }
        if (codeDevCountReceived > 0) {
            LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "MetaEvolutionEngine: Toplam CodeDev içgörüsü alındı: " << codeDevCountReceived);
        } else {
            LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "MetaEvolutionEngine: AIInsightsEngine'den hic CodeDev içgörüsü alınmadı.");
        }

        LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "MetaEvolutionEngine: LearningModule'e gonderilmeden once icgorulerin durumu (Toplam: " << insights.size() << "):");
        for (const auto& insight : insights) {
            LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "  - ID: " << insight.id << ", Type: " << static_cast<int>(insight.type) << ", Context: " << insight.context);
        }
        
        // learning_module.process_ai_insights metodu, içerdiği RL güncelleme mantığı nedeniyle RL döngüsünden ayrıldı.
        // Bu metod, doğrudan içgörüleri KnowledgeBase'e eklemek için kullanılmaya devam edecek.
        learning_module.process_ai_insights(insights);

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
        // DÜZELTİLDİ: const DynamicSequence& yerine mutable bir kopya oluşturuldu.
        DynamicSequence mutable_sequence_copy = current_sequence; 
        cryptofig_processor.process_sequence(mutable_sequence_copy, 0.01f); // learning_rate_ae dummy olarak verildi
        LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "MetaEvolutionEngine: CryptofigProcessor process_sequence tamamlandı.");
    } catch (const std::exception& e) {
        LOG_ERROR_CERR(CerebrumLux::LogLevel::ERR_CRITICAL, "MetaEvolutionEngine: CryptofigProcessor adiminda hata: " << e.what());
        return;
    } catch (...) {
        LOG_ERROR_CERR(CerebrumLux::LogLevel::ERR_CRITICAL, "MetaEvolutionEngine: CryptofigProcessor adiminda bilinmeyen hata.");
        return;
    }
    
    // YENİ EKLENDİ: Adım 5: RL Ajanı için Q-Table Güncelleme ve Eylem Seçimi (Gelişmiş)
    try {
        LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "MetaEvolutionEngine: RL Q-Table güncelleme ve eylem seçimi adımı başlatılıyor.");

        std::vector<float> next_state_embedding = current_state_embedding; // Şimdilik current_state_embedding'in kopyası

        CerebrumLux::AIAction chosen_action = CerebrumLux::AIAction::None;
        float current_reward = 0.0f;

        // Epsilon-greedy stratejisi
        float epsilon = 0.2f; // Keşif oranı (örneğin %20)
        if (CerebrumLux::SafeRNG::getInstance().get_float(0.0f, 1.0f) < epsilon) {
            // Exploration: Rastgele bir eylem seç
            std::vector<CerebrumLux::AIAction> possible_actions = {
                CerebrumLux::AIAction::RespondToUser,
                CerebrumLux::AIAction::SuggestSelfImprovement,
                CerebrumLux::AIAction::AdjustLearningRate,
                CerebrumLux::AIAction::RequestMoreData,
                CerebrumLux::AIAction::QuarantineCapsule,
                CerebrumLux::AIAction::PerformWebSearch,
                CerebrumLux::AIAction::UpdateKnowledgeBase,
                CerebrumLux::AIAction::MonitorPerformance,
                CerebrumLux::AIAction::CalibrateSensors,
                CerebrumLux::AIAction::ExecutePlan,
                CerebrumLux::AIAction::PrioritizeTask,
                CerebrumLux::AIAction::RefactorCode,
                CerebrumLux::AIAction::SuggestResearch,
                CerebrumLux::AIAction::MaximizeLearning
            };
            int random_index = static_cast<int>(CerebrumLux::SafeRNG::getInstance().get_float(0.0f, static_cast<float>(possible_actions.size() - 1)));
            chosen_action = possible_actions[random_index];
            LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MetaEvolutionEngine: Epsilon-greedy: Rastgele eylem seçildi (Exploration): " << CerebrumLux::action_to_string(chosen_action));
        } else {
            // Exploitation: Q-Table'dan en iyi eylemi seç
            // DÜZELTİLDİ: learning_module'den CryptoManager'a erişmek için getter kullanıldı ve vec_to_str overload'u çağrıldı.
            auto q_table_values_opt = learning_module.getKnowledgeBase().get_swarm_db().get_q_value_json(CerebrumLux::SwarmVectorDB::EmbeddingStateKey(learning_module.get_crypto_manager().vec_to_str(current_state_embedding)));
            
            float max_q_value = -1.0f * std::numeric_limits<float>::max(); // En küçük float değeri
            CerebrumLux::AIAction best_action_from_q = CerebrumLux::AIAction::None;

            if (q_table_values_opt) { // std::optional kontrolü
                try {
                    nlohmann::json action_map_json = nlohmann::json::parse(*q_table_values_opt);
                    for (nlohmann::json::const_iterator action_it = action_map_json.begin(); action_it != action_map_json.end(); ++action_it) {
                        float q_val = action_it.value().get<float>();
                        if (q_val > max_q_value) {
                            max_q_value = q_val;
                            best_action_from_q = CerebrumLux::string_to_action(action_it.key());
                        }
                    }
                } catch (const nlohmann::json::exception& e) {
                    LOG_ERROR_CERR(CerebrumLux::LogLevel::ERR_CRITICAL, "MetaEvolutionEngine: Q-Table JSON ayrıştırma hatası (Eylem Seçimi): " << e.what());
                }
            }
            
            if (best_action_from_q != CerebrumLux::AIAction::None) {
                chosen_action = best_action_from_q;
                LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MetaEvolutionEngine: Epsilon-greedy: Q-Table'dan en iyi eylem seçildi (Exploitation): " << CerebrumLux::action_to_string(chosen_action) << ", Q-Value: " << max_q_value);
            } else {
                chosen_action = CerebrumLux::AIAction::MonitorPerformance; // Varsayılan eylem
                LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MetaEvolutionEngine: Epsilon-greedy: Q-Table'da eylem bulunamadı, varsayılan eylem seçildi: " << CerebrumLux::action_to_string(chosen_action));
            }
        }

        // Dinamik Ödül Hesaplama
        float goal_alignment_reward = 0.0f;
        if (goal_manager.get_current_goal() == CerebrumLux::AIGoal::MaximizeLearning) {
            for (const auto& insight : insights) {
                if (insight.type == CerebrumLux::InsightType::LearningOpportunity) {
                    float urgency_val = static_cast<float>(insight.urgency) / static_cast<float>(CerebrumLux::UrgencyLevel::Critical); // Normalize
                    goal_alignment_reward += urgency_val * 0.1f;
                }
            }
        } else if (goal_manager.get_current_goal() == CerebrumLux::AIGoal::OptimizeProductivity) {
            for (const auto& insight : insights) {
                if (insight.type == CerebrumLux::InsightType::EfficiencySuggestion) {
                    float urgency_val = static_cast<float>(insight.urgency) / static_cast<float>(CerebrumLux::UrgencyLevel::Critical);
                    goal_alignment_reward += urgency_val * 0.1f;
                }
            }
        }
        
        float insight_based_reward = 0.0f;
        for (const auto& insight : insights) {
            float urgency_val = static_cast<float>(insight.urgency) / static_cast<float>(CerebrumLux::UrgencyLevel::Critical);
            if (insight.urgency == CerebrumLux::UrgencyLevel::Critical) {
                insight_based_reward += (urgency_val - 1.0f) * 0.5f; // Yüksek aciliyetli içgörü negatif ödül
            } else if (insight.urgency == CerebrumLux::UrgencyLevel::High) {
                insight_based_reward += (urgency_val - 0.5f) * 0.2f; // Orta aciliyetli içgörü daha az etki
            }
        }

        current_reward = goal_alignment_reward + insight_based_reward;
        LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MetaEvolutionEngine: Dinamik ödül hesaplandı. Toplam Ödül: " << current_reward << " (Hedef Uyum: " << goal_alignment_reward << ", İçgörü Temelli: " << insight_based_reward << ")");

        learning_module.update_q_values(current_state_embedding, chosen_action, current_reward, next_state_embedding);
        LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "MetaEvolutionEngine: RL Q-Table güncelleme tamamlandı.");
        
    } catch (const std::exception& e) {
        LOG_ERROR_CERR(CerebrumLux::LogLevel::ERR_CRITICAL, "MetaEvolutionEngine: RL Q-Table güncelleme adiminda hata: " << e.what());
    } catch (...) {
        LOG_ERROR_CERR(CerebrumLux::LogLevel::ERR_CRITICAL, "MetaEvolutionEngine: RL Q-Table güncelleme adiminda bilinmeyen hata.");
    }
    
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MetaEvolutionEngine: Meta-evolution cycle tamamlandı.");
}

} // namespace CerebrumLux