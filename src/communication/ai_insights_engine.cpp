// AIInsightsEngine.cpp
#include "ai_insights_engine.h"
#include "../core/logger.h"
#include "../core/enums.h" // InsightType, UrgencyLevel, UserIntent, AIAction, KnowledgeTopic, InsightSeverity için
#include "../core/utils.h" // intent_to_string, SafeRNG için
#include "../brain/autoencoder.h" // CryptofigAutoencoder::INPUT_DIM için
#include "../data_models/dynamic_sequence.h" // DynamicSequence için
#include <numeric> // std::accumulate için
#include <algorithm> // std::min, std::max için
#include <random> // std::uniform_int_distribution, std::normal_distribution için
#include "../core/CodeAnalyzerUtils.h" // CodeAnalyzerUtils için gerekli include eklendi
#include <filesystem> // std::filesystem için
#include <iterator> // std::make_move_iterator için (std::vector'dan taşımak için)


// ÖNEMLİ: Tüm AIInsightsEngine implementasyonu bu namespace içinde olacak.
namespace CerebrumLux {

// Yardımcı fonksiyonlar: KnowledgeTopic'i string'e dönüştürmek için
static std::string knowledge_topic_to_string(KnowledgeTopic topic) {
    switch (topic) {
        case CerebrumLux::KnowledgeTopic::SystemPerformance: return "Sistem Performansı";
        case CerebrumLux::KnowledgeTopic::LearningStrategy: return "Öğrenme Stratejisi"; 
        case CerebrumLux::KnowledgeTopic::ResourceManagement: return "Kaynak Yönetimi";
        case CerebrumLux::KnowledgeTopic::CyberSecurity: return "Siber Güvenlik";
        case CerebrumLux::KnowledgeTopic::UserBehavior: return "Kullanıcı Davranışı";
        case CerebrumLux::KnowledgeTopic::CodeDevelopment: return "Kod Geliştirme";
        case CerebrumLux::KnowledgeTopic::General: return "Genel";
        default: return "Bilinmeyen Konu";
    }
}

// Yardımcı fonksiyonlar: InsightSeverity'i UrgencyLevel'a dönüştürmek için
static CerebrumLux::UrgencyLevel insight_severity_to_urgency_level(InsightSeverity severity) {
    switch (severity) {
        case CerebrumLux::InsightSeverity::Low: return CerebrumLux::UrgencyLevel::Low;
        case CerebrumLux::InsightSeverity::Medium: return CerebrumLux::UrgencyLevel::Medium;
        case CerebrumLux::InsightSeverity::High: return CerebrumLux::UrgencyLevel::High;
        case CerebrumLux::InsightSeverity::Critical: return CerebrumLux::UrgencyLevel::Critical;
        case CerebrumLux::InsightSeverity::None: return CerebrumLux::UrgencyLevel::None;
        default: return CerebrumLux::UrgencyLevel::None;
    }
}


AIInsightsEngine::AIInsightsEngine(IntentAnalyzer& analyzer_ref, IntentLearner& learner_ref, PredictionEngine& predictor_ref,
                                 CryptofigAutoencoder& autoencoder_ref, CryptofigProcessor& cryptofig_processor_ref)
    : intent_analyzer(analyzer_ref),
      intent_learner(learner_ref),
      prediction_engine(predictor_ref),
      cryptofig_autoencoder(autoencoder_ref),
      cryptofig_processor(cryptofig_processor_ref)
{
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "AIInsightsEngine: Initialized.");

    // Projenin src dizinindeki tüm C++ kaynak ve başlık dosyalarını dinamik olarak bul
    const std::filesystem::path src_dir_path = "../src"; // Projenin kök dizinine göreceli yol
    if (std::filesystem::exists(src_dir_path) && std::filesystem::is_directory(src_dir_path)) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(src_dir_path)) {
            if (entry.is_regular_file()) {
                std::string extension = entry.path().extension().string();
                if (extension == ".cpp" || extension == ".cxx" || extension == ".cc" ||
                    extension == ".h" || extension == ".hpp") {
                    files_to_analyze.push_back(entry.path().string());
                }
            }
        }
        LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "AIInsightsEngine: Kod analizi için " << files_to_analyze.size() << " adet dosya dinamik olarak keşfedildi.");
        for (const auto& file : files_to_analyze) {
            LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "AIInsightsEngine: Analiz edilecek dosya: " << file);
        }
    } else {
        LOG_ERROR_CERR(CerebrumLux::LogLevel::ERR_CRITICAL, "AIInsightsEngine: Kod analizi için 'src' dizini bulunamadı veya erişilemiyor: " << src_dir_path.string());
        // Fallback olarak belki önceden tanımlanmış az sayıda dosyayı kullanabiliriz,
        // ancak şimdilik bu hata kritik olarak işaretlendi.
    }
}

// =========================================================================
// YARDIMCI METOTLARIN İMPLEMENTASYONLARI (Refaktör Edilmiş Kod)
// =========================================================================

// Simüle kod metriklerini günceller ve dahili üye değişkenlerini ayarlar.
void AIInsightsEngine::updateSimulatedCodeMetrics(std::chrono::system_clock::time_point now) {
    // Kod Karmaşıklığı Simülasyonu
    if (!is_on_cooldown("code_complexity_simulation", std::chrono::seconds(10))) {
        last_simulated_code_complexity += CerebrumLux::SafeRNG::getInstance().get_gaussian_float(0.0f, 0.40f);
        last_simulated_code_complexity = std::max(0.0f, std::min(1.0f, last_simulated_code_complexity));
        LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "AIInsightsEngine: Simüle Kod Karmaşıklığı Güncellendi: " << last_simulated_code_complexity);
        insight_cooldowns["code_complexity_simulation"] = now;
    }
   
    // Kod Okunabilirlik Skoru Simülasyonu
    if (!is_on_cooldown("code_readability_simulation", std::chrono::seconds(10))) {
        last_simulated_code_readability += CerebrumLux::SafeRNG::getInstance().get_gaussian_float(0.0f, 0.45f);
        last_simulated_code_readability = std::max(0.0f, std::min(1.0f, last_simulated_code_readability));
        LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "AIInsightsEngine: Simüle Kod Okunabilirlik Skoru Güncellendi: " << last_simulated_code_readability);
        insight_cooldowns["code_readability_simulation"] = now;
    }

    // Kod Optimizasyon Potansiyeli Simülasyonu
    if (!is_on_cooldown("code_optimization_potential_simulation", std::chrono::seconds(10))) {
        last_simulated_optimization_potential += CerebrumLux::SafeRNG::getInstance().get_gaussian_float(0.0f, 0.40f);
        last_simulated_optimization_potential = std::max(0.0f, std::min(1.0f, last_simulated_optimization_potential));
        LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "AIInsightsEngine: Simüle Kod Optimizasyon Potansiyeli Güncellendi: " << last_simulated_optimization_potential);
        insight_cooldowns["code_optimization_potential_simulation"] = now;
    }
}

// LOC bazlı Kod Geliştirme içgörüleri üretir
std::vector<AIInsight> AIInsightsEngine::generateCodeAnalysisInsights(const DynamicSequence& current_sequence, std::chrono::system_clock::time_point now) {
    std::vector<AIInsight> code_analysis_insights;

    if (!is_on_cooldown("code_analysis_cycle", std::chrono::seconds(30))) {
        LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "AIInsightsEngine: Kod analizi döngüsü basladi. Toplam dosya: " << files_to_analyze.size());
        for (const std::string& file_path : files_to_analyze) { 
            int loc = 0;
            try {
                loc = CerebrumLux::CodeAnalyzerUtils::countMeaningfulLinesOfCode(file_path);
            } catch (const std::exception& e) {
                LOG_ERROR_CERR(CerebrumLux::LogLevel::ERR_CRITICAL, "AIInsightsEngine: countMeaningfulLinesOfCode failed for " << file_path << ": " << e.what());
                continue;
            }
            bool loc_high_cooldown_active = is_on_cooldown("loc_high_suggestion_" + file_path, std::chrono::seconds(180));
            bool loc_critical_cooldown_active = is_on_cooldown("loc_critical_suggestion_" + file_path, std::chrono::seconds(300));

            LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "AIInsightsEngine: Kod Analizi - Dosya: " << file_path << ", LOC: " << loc << ", HighLoc Cool: " << std::boolalpha << loc_high_cooldown_active << ", CritLoc Cool: " << std::boolalpha << loc_critical_cooldown_active);
            
            // Genel LOC bazlı Kod Geliştirme Önerileri üret
            if (loc >= 180 && !is_on_cooldown("loc_high_suggestion_" + file_path, std::chrono::seconds(180))) {
                insight_cooldowns["loc_high_suggestion_" + file_path] = now; 
                code_analysis_insights.push_back(
                    AIInsight(
                        "CodeDev_HighLoc_" + CerebrumLux::generate_unique_id(),
                        "'" + file_path + "' dosyasında yüksek kod satırı sayısı (" + std::to_string(loc) + ") tespit edildi. Modülerlik iyileştirmeleri veya daha küçük fonksiyonlara bölme fırsatları olabilir.",
                        knowledge_topic_to_string(CerebrumLux::KnowledgeTopic::CodeDevelopment),
                        "'" + file_path + "' dosyasındaki sorumlulukları daha küçük ve özel modüllere ayırın. Gereksiz mantığı diğer sınıflara taşıyın.",
                        CerebrumLux::InsightType::CodeDevelopmentSuggestion,
                        insight_severity_to_urgency_level(CerebrumLux::InsightSeverity::High),
                        current_sequence.latent_cryptofig_vector,
                        {current_sequence.id},
                        std::string(file_path),
                        current_sequence.id // source_cryptofig_id olarak current_sequence.id kullanıldı
                        )
                );
                LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "AIInsightsEngine: 'CodeDev_HighLoc_' içgörüsü eklendi. Dosya: " << file_path << ", Vector boyutu: " << code_analysis_insights.size());
            } else {
                LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "AIInsightsEngine: LOC bazlı (High) Kod Geliştirme Önerisi koşulu saglanmadi. Dosya: " << file_path << ", LOC: " << loc << " (Eşik >180), Cooldown: " << std::boolalpha << is_on_cooldown("loc_high_suggestion_" + file_path, std::chrono::seconds(180)));
            }
            
            // Çok yüksek LOC için kritik öneri (farklı bir eşik)
            if (loc >= 300 && !is_on_cooldown("loc_critical_suggestion_" + file_path, std::chrono::seconds(300))) { 
                insight_cooldowns["loc_critical_suggestion_" + file_path] = now; 
                code_analysis_insights.push_back(
                    AIInsight(
                        "CodeDev_CriticalLoc_" + CerebrumLux::generate_unique_id(),
                        "'" + file_path + "' dosyasında çok yüksek kod satırı sayısı (" + std::to_string(loc) + ") tespit edildi. Acil modülerlik ve refaktör ihtiyacı var!",
                        knowledge_topic_to_string(CerebrumLux::KnowledgeTopic::CodeDevelopment),
                        "main.cpp'deki sorumlulukları daha küçük ve özel modüllere ayırın. Gereksiz mantığı diğer sınıflara taşıyın.",
                        CerebrumLux::InsightType::CodeDevelopmentSuggestion,
                        insight_severity_to_urgency_level(CerebrumLux::InsightSeverity::Critical),
                        current_sequence.latent_cryptofig_vector,
                        {current_sequence.id},
                        std::string(file_path),
                        current_sequence.id // source_cryptofig_id olarak current_sequence.id kullanıldı
                    )
                ); 
                LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "AIInsightsEngine: 'CodeDev_CriticalLoc_' içgörüsü eklendi. Dosya: " << file_path << ", Vector boyutu: " << code_analysis_insights.size());
            } else {
                LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "AIInsightsEngine: LOC bazlı (Critical) Kod Geliştirme Önerisi koşulu saglanmadi. Dosya: " << file_path << ", LOC: " << loc << " (Eşik >300), Cooldown: " << std::boolalpha << is_on_cooldown("loc_critical_suggestion_" + file_path, std::chrono::seconds(300)));
            }
            LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "AIInsightsEngine: Dosya analizi tamamlandi: " << file_path);
        }
        LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "AIInsightsEngine: Kod analizi döngüsü tamamlandı, cooldown güncelleniyor.");
        insight_cooldowns["code_analysis_cycle"] = now;
        LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "AIInsightsEngine: Code analizi döngüsü cooldown güncellendi.");
        LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "AIInsightsEngine: LOC Analizi Döngüsü Tamamlandı. Üretilen CodeDev içgörüleri: " << code_analysis_insights.size() << ", Bayrak: " << std::boolalpha << !code_analysis_insights.empty());
    } else {
        LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "AIInsightsEngine: Kod Analizi Döngüsü atlandi (Cooldown aktif).");
    }

    return code_analysis_insights;
}

// Simüle Metrik bazlı Kod Geliştirme İçgörüleri üretir
std::vector<AIInsight> AIInsightsEngine::generateSimulatedMetricCodeDevelopmentInsights(const DynamicSequence& current_sequence, std::chrono::system_clock::time_point now) {
    std::vector<AIInsight> simulated_metric_code_dev_insights;

    LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "AIInsightsEngine: Simule kod metrikleri bazli icgoru kontrolleri basliyor.");

    // 1. Yüksek Karmaşıklık ve Düşük Okunabilirlik
    bool high_complexity_low_readability_eval = (last_simulated_code_complexity > 0.6f && last_simulated_code_readability < 0.4f); 
    LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "AIInsightsEngine: CodeDev (Yüksek Karmaşıklık/Düşük Okunabilirlik) Koşul Durumu: " << std::boolalpha << high_complexity_low_readability_eval << ", Cooldown Aktif: " << std::boolalpha << is_on_cooldown("high_complexity_low_readability_suggestion", std::chrono::seconds(60)) << ", Comp: " << last_simulated_code_complexity << ", Read: " << last_simulated_code_readability);
    if (high_complexity_low_readability_eval && !is_on_cooldown("high_complexity_low_readability_suggestion", std::chrono::seconds(60))) {
        insight_cooldowns["high_complexity_low_readability_suggestion"] = now;
        simulated_metric_code_dev_insights.push_back(
            AIInsight(
                "CodeDev_HighComplexityLowReadability_" + CerebrumLux::generate_unique_id(),
                "Kritik seviyede yüksek kod karmaşıklığı (" + std::to_string(last_simulated_code_complexity) + "), düşük okunabilirlik (" + std::to_string(last_simulated_code_readability) + ") tespit edildi. Modülerlik iyileştirmeleri ve refaktör ACİL!",
                knowledge_topic_to_string(CerebrumLux::KnowledgeTopic::CodeDevelopment),
                "Kritik karmaşıklığa sahip fonksiyonları/sınıfları belirleyin, sorumluğu tek olan parçalara bölün ve kod yorumlamasını artırın. İsimlendirme standartlarını gözden geçirin.",
                CerebrumLux::InsightType::CodeDevelopmentSuggestion,
                insight_severity_to_urgency_level(CerebrumLux::InsightSeverity::Critical),
                current_sequence.latent_cryptofig_vector,
                {current_sequence.id},
                "src/communication/ai_insights_engine.cpp",
                current_sequence.id // source_cryptofig_id olarak current_sequence.id kullanıldı
            )
        );
        LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "AIInsightsEngine: 'CodeDev_HighComplexityLowReadability_' içgörüsü eklendi. Vector boyutu: " << simulated_metric_code_dev_insights.size());
    } else {
        LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "AIInsightsEngine: Simule Metrik (Yüksek Karmaşıklık/Düşük Okunabilirlik) koşulu saglanmadi. Comp: " << last_simulated_code_complexity << ", Read: " << last_simulated_code_readability);
    }

    // 2. Yüksek Optimizasyon Potansiyeli
    bool high_optimization_potential_eval = (last_simulated_optimization_potential > 0.6f); 
    LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "AIInsightsEngine: CodeDev (Yüksek Optimizasyon Potansiyeli) Koşul Durumu: " << std::boolalpha << high_optimization_potential_eval << ", Cooldown Aktif: " << std::boolalpha << is_on_cooldown("high_optimization_potential_suggestion", std::chrono::seconds(90)) << ", Opt: " << last_simulated_optimization_potential);
    if (high_optimization_potential_eval && !is_on_cooldown("high_optimization_potential_suggestion", std::chrono::seconds(90))) {
        insight_cooldowns["high_optimization_potential_suggestion"] = now;
        simulated_metric_code_dev_insights.push_back(
            AIInsight(
                "CodeDev_HighOptimizationPotential_" + CerebrumLux::generate_unique_id(),
                "Yüksek performans optimizasyon potansiyeli tespit edildi (" + std::to_string(last_simulated_optimization_potential) + "). Bazı döngüler veya algoritmalar iyileştirilebilir.",
                knowledge_topic_to_string(CerebrumLux::KnowledgeTopic::CodeDevelopment),
                "Sıkça çağrılan ve zaman alan kod bloklarını profilleyin. Alternatif veri yapıları veya algoritmalar kullanarak performansı artırın. (Mevcut Potansiyel: " + std::to_string(last_simulated_optimization_potential) + ")",
                CerebrumLux::InsightType::CodeDevelopmentSuggestion,
                insight_severity_to_urgency_level(CerebrumLux::InsightSeverity::High),
                current_sequence.latent_cryptofig_vector,
                {current_sequence.id},
                "src/brain/cryptofig_processor.cpp",
                current_sequence.id // source_cryptofig_id olarak current_sequence.id kullanıldı
            )
        );
        LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "AIInsightsEngine: 'CodeDev_HighOptimizationPotential_' içgörüsü eklendi. Vector boyutu: " << simulated_metric_code_dev_insights.size());
    } else {
        LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "AIInsightsEngine: Simule Metrik (Yüksek Optimizasyon Potansiyeli) koşulu saglanmadi. Opt: " << last_simulated_optimization_potential);
    }

    // 3. Genel Modülerlik/Refaktör Fırsatları (Orta Karmaşıklık/Okunabilirlik)
    bool medium_complexity_readability_eval = (last_simulated_code_complexity > 0.5f && last_simulated_code_readability < 0.7f);
    LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "AIInsightsEngine: CodeDev (Orta Karmaşıklık/Okunabilirlik) Koşul Durumu: " << std::boolalpha << medium_complexity_readability_eval << ", Cooldown Aktif: " << std::boolalpha << is_on_cooldown("medium_complexity_readability_suggestion", std::chrono::seconds(120)) << ", Comp: " << last_simulated_code_complexity << ", Read: " << last_simulated_code_readability);
    if (medium_complexity_readability_eval && !is_on_cooldown("medium_complexity_readability_suggestion", std::chrono::seconds(120))) {
        insight_cooldowns["medium_complexity_readability_suggestion"] = now;
        simulated_metric_code_dev_insights.push_back(
            AIInsight(
                "CodeDev_MediumComplexityReadability_" + CerebrumLux::generate_unique_id(),
                "Kod tabanında potansiyel modülerlik iyileştirmeleri veya refaktör fırsatları olabilir (Karmaşıklık: " + std::to_string(last_simulated_code_complexity) + ", Okunabilirlik: " + std::to_string(last_simulated_code_readability) + ").",
                knowledge_topic_to_string(CerebrumLux::KnowledgeTopic::CodeDevelopment),
                "Kod tabanını analiz ederek potansiyel modülerlik veya refaktör alanlarını belirleyin. Benzer işlevselliğe sahip parçaları soyutlayın.",
                CerebrumLux::InsightType::CodeDevelopmentSuggestion,
                insight_severity_to_urgency_level(CerebrumLux::InsightSeverity::Medium),
                current_sequence.latent_cryptofig_vector,
                {current_sequence.id},
                "src/learning/LearningModule.cpp",
                current_sequence.id // source_cryptofig_id olarak current_sequence.id kullanıldı
            )
        );
        LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "AIInsightsEngine: 'CodeDev_MediumComplexityReadability_' içgörüsü eklendi. Vector boyutu: " << simulated_metric_code_dev_insights.size());
    } else {
        LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "AIInsightsEngine: Simule Metrik (Orta Karmaşıklık/Okunabilirlik) koşulu saglanmadi. Comp: " << last_simulated_code_complexity << ", Read: " << last_simulated_code_readability);
    }

    // 4. İyi Durumda Ama Küçük İyileştirmeler (Düşük Karmaşıklık, Yüksek Okunabilirlik)
    bool minor_improvement_eval = (last_simulated_code_readability > 0.4f); 
    LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "AIInsightsEngine: CodeDev (Minor Improvement) Koşul Durumu: " << std::boolalpha << minor_improvement_eval << ", Cooldown Aktif: " << std::boolalpha << is_on_cooldown("minor_code_improvement_suggestion", std::chrono::seconds(10)) << ", Comp: " << last_simulated_code_complexity << ", Read: " << last_simulated_code_readability << ", Opt: " << last_simulated_optimization_potential);
    if (minor_improvement_eval && !is_on_cooldown("minor_code_improvement_suggestion", std::chrono::seconds(10))) {
        insight_cooldowns["minor_code_improvement_suggestion"] = now;
        simulated_metric_code_dev_insights.push_back(
            AIInsight(
                "CodeDev_MinorImprovement_" + CerebrumLux::generate_unique_id(),
                "Mevcut kod tabanı iyi durumda. Ancak küçük çaplı stil veya dokümantasyon iyileştirmeleri yapılabilir (Karmaşıklık: " + std::to_string(last_simulated_code_complexity) + ", Okunabilirlik: " + std::to_string(last_simulated_code_readability) + ").",
                knowledge_topic_to_string(CerebrumLux::KnowledgeTopic::CodeDevelopment),
                "Kodlama stil rehberini gözden geçirin ve tutarlılığı artırın. Eksik dokümantasyonu tamamlayın veya mevcut yorumları geliştirin.",
                CerebrumLux::InsightType::CodeDevelopmentSuggestion,
                insight_severity_to_urgency_level(CerebrumLux::InsightSeverity::Low),
                current_sequence.latent_cryptofig_vector,
                {current_sequence.id},
                "src/gui/MainWindow.cpp",
                current_sequence.id // source_cryptofig_id olarak current_sequence.id kullanıldı
            )
        );
        LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "AIInsightsEngine: 'CodeDev_MinorImprovement_' içgörüsü eklendi. Vector boyutu: " << simulated_metric_code_dev_insights.size());
    } else {
        LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "AIInsightsEngine: Simule Metrik (Minor Improvement) koşulu saglanmadi. Read: " << last_simulated_code_readability);
    }

    return simulated_metric_code_dev_insights;
}

// Genel içgörüler üretir (CodeDev içgörüler üretilmediğinde kullanılır)
std::vector<AIInsight> AIInsightsEngine::generateGeneralNonCodeDevelopmentInsights(const DynamicSequence& current_sequence, std::chrono::system_clock::time_point now) {
    std::vector<AIInsight> general_insights;

    // --- Genel Performans Metriği (Grafik Besleme) ---
    if (!current_sequence.latent_cryptofig_vector.empty() && !is_on_cooldown("ai_confidence_graph", std::chrono::seconds(5))) {
        float avg_latent_confidence = 0.0f;
        for (float val : current_sequence.latent_cryptofig_vector) {
            avg_latent_confidence += val;
        }
        avg_latent_confidence /= current_sequence.latent_cryptofig_vector.size();
        float normalized_confidence = std::max(0.0f, std::min(1.0f, avg_latent_confidence));

        general_insights.push_back(
            AIInsight(
                "AI_Confidence_" + std::to_string(current_sequence.timestamp_utc.time_since_epoch().count()),
                "AI sisteminin anlık güven seviyesi: " + std::to_string(normalized_confidence),
                "Sistem Genel Performans Metriği",
                "Grafiği gözlemlemeye devam et.",
                CerebrumLux::InsightType::None, CerebrumLux::UrgencyLevel::None,
                current_sequence.latent_cryptofig_vector,
                {current_sequence.id}, // related_capsule_ids
                "", // code_file_path
                ""
            )
        );
        insight_cooldowns["ai_confidence_graph"] = now;
        LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "AIInsightsEngine: AI Güven Seviyesi içgörüsü (grafik için) üretildi: " << normalized_confidence << ". Toplam: " << general_insights.size());
    } else {
        LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "AIInsightsEngine: AI Güven Seviyesi içgörüsü koşulu saglanmadi (Cooldown aktif).");
    }

    // Eğer yukarıdaki koşulların hiçbiriyle içgörü üretilemediyse ve stabil durum içgörüsü cooldown'da değilse (LOG SEVİYESİ TRACE)
    if (general_insights.empty() && !is_on_cooldown("stable_state", std::chrono::seconds(300))) {
        general_insights.push_back(
            AIInsight(
                "StableState_" + std::to_string(current_sequence.timestamp_utc.time_since_epoch().count()),
                "İç durumum stabil görünüyor. Yeni öğrenme fırsatları için hazırım.",
                "Genel Durum",
                "Yeni özellik geliştirme veya derinlemesine öğrenme moduna geç.",
                CerebrumLux::InsightType::None,
                insight_severity_to_urgency_level(CerebrumLux::InsightSeverity::Low),
                current_sequence.latent_cryptofig_vector, // Eğer boş değilse latent cryptofig'i ata
                {current_sequence.id}, // related_capsule_ids
                ""
            )
        );
        insight_cooldowns["stable_state"] = now;
        LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "AIInsightsEngine: 'StableState_' içgörüsü eklendi. Toplam: " << general_insights.size());
    } else if (!general_insights.empty()) {
        LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "AIInsightsEngine: StableState içgörüsü eklenmedi çünkü başka içgörüler mevcut. Toplam mevcut içgörü: " << general_insights.size());
    } else {
        LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "AIInsightsEngine: StableState içgörüsü eklenmedi (cooldown aktif).");
    }

    // Yardımcı içgörü üretim metotlarını çağır
    AIInsight insight_from_helper;

    insight_from_helper = generate_reconstruction_error_insight(current_sequence);
    if (!insight_from_helper.id.empty()) general_insights.push_back(insight_from_helper);

    insight_from_helper = generate_learning_rate_insight(current_sequence);
    if (!insight_from_helper.id.empty()) general_insights.push_back(insight_from_helper);

    insight_from_helper = generate_system_resource_insight(current_sequence);
    if (!insight_from_helper.id.empty()) general_insights.push_back(insight_from_helper);

    insight_from_helper = generate_network_activity_insight(current_sequence);
    if (!insight_from_helper.id.empty()) general_insights.push_back(insight_from_helper);

    insight_from_helper = generate_application_context_insight(current_sequence);
    if (!insight_from_helper.id.empty()) general_insights.push_back(insight_from_helper);

    insight_from_helper = generate_unusual_behavior_insight(current_sequence);
    if (!insight_from_helper.id.empty()) general_insights.push_back(insight_from_helper);

    // Performans Anormalliği
    if (!current_sequence.statistical_features_vector.empty() && current_sequence.statistical_features_vector[0] > 0.8) {
        general_insights.push_back(
            AIInsight(
                "PerformanceAnomaly_" + std::to_string(now.time_since_epoch().count()),
                "Sistemde potansiyel performans anormalliği tespit edildi.",
                knowledge_topic_to_string(CerebrumLux::KnowledgeTopic::SystemPerformance),
                "Performans izleme araçlarını kontrol edin ve anormal süreçleri belirleyin.",
                CerebrumLux::InsightType::PerformanceAnomaly,
                insight_severity_to_urgency_level(CerebrumLux::InsightSeverity::High),
                current_sequence.latent_cryptofig_vector,
                {current_sequence.id},
                "", // code_file_path
                current_sequence.id // source_cryptofig_id
            )
        );
        LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "AIInsightsEngine: 'PerformanceAnomaly_' içgörüsü eklendi. Toplam: " << general_insights.size());
    }

    // Öğrenme Fırsatı
    if (!current_sequence.latent_cryptofig_vector.empty() && current_sequence.latent_cryptofig_vector[0] < 0.2) {
        general_insights.push_back(
            AIInsight(
                "LearningOpportunity_" + std::to_string(current_sequence.timestamp_utc.time_since_epoch().count()),
                "Yeni bir öğrenme fırsatı belirlendi. Bilgi tabanının genişletilmesi önerilir.",
                knowledge_topic_to_string(CerebrumLux::KnowledgeTopic::LearningStrategy),
                "Mevcut bilgi tabanını gözden geçirin ve yeni öğrenme kaynakları arayın.",
                CerebrumLux::InsightType::LearningOpportunity,
                insight_severity_to_urgency_level(CerebrumLux::InsightSeverity::Medium),
                current_sequence.latent_cryptofig_vector,
                {current_sequence.id},
                "", // code_file_path
                current_sequence.id // source_cryptofig_id
            )
        );
        LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "AIInsightsEngine: 'LearningOpportunity_' içgörüsü eklendi. Toplam: " << general_insights.size());
    }

    // Kaynak Optimizasyonu
    if (!current_sequence.statistical_features_vector.empty() && current_sequence.statistical_features_vector[1] > 0.9) {
        general_insights.push_back(
            AIInsight(
                "ResourceOptimization_" + std::to_string(current_sequence.timestamp_utc.time_since_epoch().count()),
                "Yüksek kaynak kullanımı tespit edildi. Optimizasyon önerileri değerlendirilmeli.",
                knowledge_topic_to_string(CerebrumLux::KnowledgeTopic::ResourceManagement),
                "Arka plan uygulamalarını kontrol edin veya gereksiz isleri durdurun.",
                CerebrumLux::InsightType::ResourceOptimization,
                insight_severity_to_urgency_level(CerebrumLux::InsightSeverity::Medium), // UrgencyLevel
                current_sequence.latent_cryptofig_vector, // Eğer boş değilse latent cryptofig'i ata
                {current_sequence.id},
                ""
            )
        );
        LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "AIInsightsEngine: 'ResourceOptimization_' içgörüsü eklendi. Toplam: " << general_insights.size());
    }

    // Güvenlik Uyarısı
    if (!current_sequence.statistical_features_vector.empty() && current_sequence.statistical_features_vector[2] < 0.1) {
        general_insights.push_back(
            AIInsight(
                "SecurityAlert_" + std::to_string(current_sequence.timestamp_utc.time_since_epoch().count()),
                "Potansiel güvenlik açığı veya anormal davranış tespit edildi.",
                knowledge_topic_to_string(CerebrumLux::KnowledgeTopic::CyberSecurity),
                "Sistem loglarını inceleyin ve güvenlik taraması yapın.",
                CerebrumLux::InsightType::SecurityAlert,
                insight_severity_to_urgency_level(CerebrumLux::InsightSeverity::High), // Düzeltildi: UrgencyLevel yerine InsightSeverity
                current_sequence.latent_cryptofig_vector,
                {current_sequence.id},
                "", // code_file_path
                current_sequence.id // source_cryptofig_id
            )
        );
        LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "AIInsightsEngine: 'SecurityAlert_' içgörüsü eklendi. Toplam: " << general_insights.size());
    }

    // Kullanıcı Bağlamı
    if (!current_sequence.latent_cryptofig_vector.empty() && current_sequence.latent_cryptofig_vector[1] > 0.7) {
        general_insights.push_back(
            AIInsight(
                "UserContext_" + std::to_string(current_sequence.timestamp_utc.time_since_epoch().count()),
                "Kullanıcı bağlamında önemli bir değişiklik gözlemlendi. Adaptif yanıtlar için analiz ediliyor.",
                knowledge_topic_to_string(CerebrumLux::KnowledgeTopic::UserBehavior),
                "Kullanıcının mevcut aktivitesine göre adaptif yanıtlar hazırlayın.",
                CerebrumLux::InsightType::UserContext,
                insight_severity_to_urgency_level(CerebrumLux::InsightSeverity::Low), // Düzeltildi: UrgencyLevel yerine InsightSeverity
                current_sequence.latent_cryptofig_vector,
                {current_sequence.id},
                "", // code_file_path
                current_sequence.id // source_cryptofig_id
            )
        );
        LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "AIInsightsEngine: 'UserContext_' içgörüsü eklendi. Toplam: " << general_insights.size());
    }

    return general_insights;
}

// === Insight generation ana metodu (Refaktör Edilmiş) ===
std::string AIInsightsEngine::generate_insights(const DynamicSequence& current_sequence) {
    LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "AIInsightsEngine::generate_insights: Yeni içgörüler üretiliyor.");
    auto now = std::chrono::system_clock::now();
    std::vector<AIInsight> insights_vector;
    bool code_dev_insight_generated = false;

    // Adım 1: Simüle kod metriklerini güncelle
    updateSimulatedCodeMetrics(now);
    LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "AIInsightsEngine: Simüle kod metrikleri güncellendi.");

    // Adım 2: Kod Geliştirme İçgörülerini oluştur ve önceliklendir
    std::vector<AIInsight> code_analysis_insights = generateCodeAnalysisInsights(current_sequence, now);
    std::vector<AIInsight> simulated_metric_code_dev_insights = generateSimulatedMetricCodeDevelopmentInsights(current_sequence, now);

    insights_vector.insert(insights_vector.end(), std::make_move_iterator(code_analysis_insights.begin()), std::make_move_iterator(code_analysis_insights.end()));
    insights_vector.insert(insights_vector.end(), std::make_move_iterator(simulated_metric_code_dev_insights.begin()), std::make_move_iterator(simulated_metric_code_dev_insights.end()));

    if (!insights_vector.empty()) {
        code_dev_insight_generated = true;
    }

    if (code_dev_insight_generated) {
        LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "AIInsightsEngine::generate_insights: CodeDev içgörüler üretildi (" << insights_vector.size() << " adet). Diğer içgörüler ATLANDI.\n");
        for (const auto& insight : insights_vector) {
            LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "AIInsightsEngine: DÖNÜŞ İÇGÖRÜSÜ (Özet): ID=" << insight.id
                                  << ", Type=" << static_cast<int>(insight.type)
                                  << ", Context=" << insight.context << ", FilePath=" << insight.code_file_path);
            if (insight.type == CerebrumLux::InsightType::CodeDevelopmentSuggestion) {
                LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "AIInsightsEngine: !!! DÖNÜŞ İÇGÖRÜSÜ CodeDev TESPİT EDİLDİ: ID=" << insight.id << ", FilePath=" << insight.code_file_path);
            }
        }
        nlohmann::json j = insights_vector;
        std::string result_json_str = j.dump();
        LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "AIInsightsEngine: DÖNÜŞ JSON STRING (Tam - CodeDev Öncelikli): " << result_json_str.substr(0, std::min(result_json_str.length(), (size_t)500)));
        return result_json_str;
    }
    // Adım 3: Eğer CodeDev içgörüsü üretilmediyse, genel içgörüler üret
    else {
        std::vector<AIInsight> general_non_code_dev_insights = generateGeneralNonCodeDevelopmentInsights(current_sequence, now);
        insights_vector.insert(insights_vector.end(), std::make_move_iterator(general_non_code_dev_insights.begin()), std::make_move_iterator(general_non_code_dev_insights.end()));
    }
    
    LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "AIInsightsEngine::generate_insights: İçgörü üretimi bitti. Sayı: " << insights_vector.size() << "\n");

    // Metottan dönmeden önce tüm üretilen içgörüleri özetle (LOG SEVİYESİ TRACE)
    for (const auto& insight : insights_vector) {
        LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "AIInsightsEngine: DÖNÜŞ İÇGÖRÜSÜ (Özet): ID=" << insight.id
                                  << ", Type=" << static_cast<int>(insight.type)
                                  << ", Context=" << insight.context << ", FilePath=" << insight.code_file_path);
        if (insight.type == CerebrumLux::InsightType::CodeDevelopmentSuggestion) {
            LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "AIInsightsEngine: !!! DÖNÜŞ İÇGÖRÜSÜ CodeDev TESPİT EDİLDİ: ID=" << insight.id << ", FilePath=" << insight.code_file_path);
        }                          
    }
    
    nlohmann::json j = insights_vector;
    return j.dump();
}

// Cooldown kontrolü
bool AIInsightsEngine::is_on_cooldown(const std::string& key, std::chrono::seconds cooldown_duration) const {
    auto it = insight_cooldowns.find(key);
    if (it != insight_cooldowns.end()) {
        auto elapsed = std::chrono::system_clock::now() - it->second;
        if (elapsed < cooldown_duration) {
            return true;
        }
    }
    return false;
}

float AIInsightsEngine::calculate_autoencoder_reconstruction_error(const std::vector<float>& statistical_features) const {
    if (statistical_features.empty() || statistical_features.size() != CryptofigAutoencoder::INPUT_DIM) {
        LOG_DEFAULT(CerebrumLux::LogLevel::WARNING, "AIInsightsEngine: Reconstruction error hesaplanamadi, statistical_features boş veya boyut uyuşmuyor.");
        return 1.0f; // Yüksek bir hata değeri döndür
    }
    std::vector<float> reconstructed = this->cryptofig_autoencoder.reconstruct(statistical_features);
    return this->cryptofig_autoencoder.calculate_reconstruction_error(statistical_features, reconstructed);
}

IntentAnalyzer& AIInsightsEngine::get_analyzer() const {
    return this->intent_analyzer;
}

// === Mevcut Yardımcı İçgörü Üretim Metotları (generateGeneralNonCodeDevelopmentInsights tarafından çağrılacaktır) ===
// Bu metotların implementasyonları daha kısa tutulmuştur çünkü generateGeneralNonCodeDevelopmentInsights'a taşınacaklar.
// Orijinal içeriklerini korumaları önemlidir.

AIInsight AIInsightsEngine::generate_reconstruction_error_insight(const DynamicSequence& current_sequence) {
    auto now = std::chrono::system_clock::now();
    if (is_on_cooldown("reconstruction_error_insight", std::chrono::seconds(10))) return {};

    if (!current_sequence.statistical_features_vector.empty() && current_sequence.statistical_features_vector.size() == CryptofigAutoencoder::INPUT_DIM) {
        float reconstruction_error = calculate_autoencoder_reconstruction_error(current_sequence.statistical_features_vector);

        if (reconstruction_error > 0.1f) {
            this->insight_cooldowns["reconstruction_error_insight"] = now;
            return {"RecError_" + std::to_string(current_sequence.timestamp_utc.time_since_epoch().count()),
                    "Kriptofig analizim, mevcut sensor verisindeki bazi desenleri tam olarak ogrenemiyor. Hata: " + std::to_string(reconstruction_error),
                    "Veri Temsili", "Autoencoder ogrenme oranini artir, daha fazla epoch calistir.",
                    CerebrumLux::InsightType::PerformanceAnomaly, CerebrumLux::UrgencyLevel::High, current_sequence.latent_cryptofig_vector, {current_sequence.id}, ""};
        } else if (reconstruction_error < 0.01f) {
            this->insight_cooldowns["reconstruction_error_insight"] = now;
            return {"RecErrorOpt_" + std::to_string(current_sequence.timestamp_utc.time_since_epoch().count()),
                    "Autoencoder'im veriyi cok iyi yeniden yapilandiriyor. Latent uzayi kucultme onerisi.",
                    "Verimlilik", "Autoencoder latent boyutunu dusurme veya budama onerisi.",
                    CerebrumLux::InsightType::EfficiencySuggestion, CerebrumLux::UrgencyLevel::Low, current_sequence.latent_cryptofig_vector, {current_sequence.id}, ""};
        }
    } else {
        if (!is_on_cooldown("no_stats_vector", std::chrono::seconds(15))) {
            this->insight_cooldowns["no_stats_vector"] = now;
            return {"NoStats_" + std::to_string(current_sequence.timestamp_utc.time_since_epoch().count()),
                    "Istatistiksel ozellik vektoru bos, Autoencoder performansi hakkinda yorum yapamiyorum.",
                    "Veri Kalitesi", "Giris verisi akisini kontrol et.",
                    CerebrumLux::InsightType::PerformanceAnomaly, CerebrumLux::UrgencyLevel::Medium, {}, {current_sequence.id}, ""};
        }
    }
    return {};
}

AIInsight AIInsightsEngine::generate_learning_rate_insight(const DynamicSequence& current_sequence) {
    auto now = std::chrono::system_clock::now();
    if (is_on_cooldown("learning_rate_insight", std::chrono::seconds(20))) return {};
    float current_learning_rate = this->intent_learner.get_learning_rate();
    float intent_confidence = this->intent_analyzer.get_last_confidence();
    if (intent_confidence > 0.8f) {
         this->insight_cooldowns["learning_rate_insight"] = now;
         return {"LrnRateOpt_" + std::to_string(current_sequence.timestamp_utc.time_since_epoch().count()),
                "Niyet algilama güvenim yuksek. Ogrenme oranini optimize edebilirim.",
                "Ogrenme Stratejisi", "Ogrenme oranini azaltma veya adaptif olarak ayarlama onerisi.",
                CerebrumLux::InsightType::EfficiencySuggestion, CerebrumLux::UrgencyLevel::Low, current_sequence.latent_cryptofig_vector, {current_sequence.id}, ""};
    } else if (intent_confidence < 0.6f) {
        this->insight_cooldowns["learning_rate_insight"] = now;
        return {"LrnRateBoost_" + std::to_string(current_sequence.timestamp_utc.time_since_epoch().count()),
                "Niyet algilamamda güvenim düşük. Daha hizli ogrenmek icin ogrenme oranimi artirmaliyim.",
                "Ogrenme Stratejisi", "Ogrenme oranini artirma önerisi.",
                CerebrumLux::InsightType::LearningOpportunity, CerebrumLux::UrgencyLevel::High, current_sequence.latent_cryptofig_vector, {current_sequence.id}, ""};
    }
    return {};
}

AIInsight AIInsightsEngine::generate_system_resource_insight(const DynamicSequence& current_sequence) {
    auto now = std::chrono::system_clock::now();
    if (is_on_cooldown("system_resource_insight", std::chrono::seconds(15))) return {};
    if (current_sequence.current_cpu_usage > 80 || current_sequence.current_ram_usage > 90) {
        this->insight_cooldowns["system_resource_insight"] = now;
        return {"SysResHigh_" + std::to_string(current_sequence.timestamp_utc.time_since_epoch().count()),
                "Sistem kaynaklari yÜksek kullanimda. Uygulama performansi etkilenebilir. CPU: " + std::to_string(current_sequence.current_cpu_usage) + "%, RAM: " + std::to_string(current_sequence.current_ram_usage) + "%",
                "Sistem Performansi", "Arka plan uygulamalarini kontrol et veya gereksiz isleri durdur.",
                CerebrumLux::InsightType::ResourceOptimization, CerebrumLux::UrgencyLevel::Medium, {}, {current_sequence.id}, ""};
    } else if (current_sequence.current_cpu_usage < 20 && current_sequence.current_ram_usage < 30) {
        this->insight_cooldowns["system_resource_insight"] = now;
        return {"SysResLow_" + std::to_string(current_sequence.timestamp_utc.time_since_epoch().count()),
                "Sistem kaynaklari boşta. Performansli isler icin hazir.",
                "Sistem Performansi", "Yeni görevler atayabilirsin.",
                CerebrumLux::InsightType::EfficiencySuggestion, CerebrumLux::UrgencyLevel::Low, {}, {current_sequence.id}, ""};
    }
    return {};
}

AIInsight AIInsightsEngine::generate_network_activity_insight(const DynamicSequence& current_sequence) {
    auto now = std::chrono::system_clock::now();
    if (is_on_cooldown("network_activity_insight", std::chrono::seconds(20))) return {};
    if (current_sequence.current_network_active && current_sequence.network_activity_level > 70) {
        this->insight_cooldowns["network_activity_insight"] = now;
        return {"NetActHigh_" + std::to_string(current_sequence.timestamp_utc.time_since_epoch().count()),
                "YÜksek ağ aktivitesi tespit edildi. Protokol: " + current_sequence.network_protocol + ", Seviye: " + std::to_string(current_sequence.network_activity_level) + "%",
                "Ağ Güvenliği/Performansı", "Ağ trafiğini incele.",
                CerebrumLux::InsightType::SecurityAlert, CerebrumLux::UrgencyLevel::Medium, {}, {current_sequence.id}, ""};
    } else if (!current_sequence.current_network_active) {
         this->insight_cooldowns["network_activity_insight"] = now;
         return {"NoNet_" + std::to_string(current_sequence.timestamp_utc.time_since_epoch().count()),
                "Ağ bağlantısı yok veya çok düşük aktivite. İnternet erişimini kontrol et.",
                "Sistem Durumu", "Ağ bağlantısını kontrol etme önerisi.",
                CerebrumLux::InsightType::ResourceOptimization, CerebrumLux::UrgencyLevel::Low, {}, {current_sequence.id}, ""};
    }
    return {};
}

AIInsight AIInsightsEngine::generate_application_context_insight(const DynamicSequence& current_sequence) {
    auto now = std::chrono::system_clock::now();
    if (is_on_cooldown("application_context_insight", std::chrono::seconds(30))) return {};
    if (!current_sequence.current_application_context.empty() && current_sequence.current_application_context != "UnknownApp") {
        this->insight_cooldowns["application_context_insight"] = now;
        return {"AppCtx_" + std::to_string(current_sequence.timestamp_utc.time_since_epoch().count()),
                "Aktif uygulama bağlamı: " + current_sequence.current_application_context,
                "Kullanıcı Bağlamı", "Kullanıcının aktif olduğu uygulamayı dikkate alarak yardımcı ol.",
                CerebrumLux::InsightType::None, CerebrumLux::UrgencyLevel::Low, {}, {current_sequence.id}, ""};
    }
    return {};
}

AIInsight AIInsightsEngine::generate_unusual_behavior_insight(const DynamicSequence& current_sequence) {
    auto now = std::chrono::system_clock::now();
    if (is_on_cooldown("unusual_behavior_insight", std::chrono::seconds(25))) return {};
    if (CerebrumLux::SafeRNG::getInstance().get_int(0, 100) < 10) {
        this->insight_cooldowns["unusual_behavior_insight"] = now;
        return {"UnusualBehavior_" + std::to_string(current_sequence.timestamp_utc.time_since_epoch().count()),
                "Sistemde alışılmadık bir davranış tespit edildi. Daha detaylı analiz gerekiyor.",
                "Güvenlik", "Sistem loglarını incele.",
                CerebrumLux::InsightType::SecurityAlert, CerebrumLux::UrgencyLevel::Critical, current_sequence.latent_cryptofig_vector, {current_sequence.id}, ""};
    }
    return {};
}

} // namespace CerebrumLux