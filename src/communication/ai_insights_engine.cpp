#include "ai_insights_engine.h"
#include "../core/logger.h"
#include "../core/enums.h" // InsightType, UrgencyLevel, UserIntent, AIAction için
#include "../core/utils.h" // intent_to_string, SafeRNG için
#include "../brain/autoencoder.h" // CryptofigAutoencoder::INPUT_DIM için
#include <numeric> // std::accumulate için
#include <algorithm> // std::min, std::max için
#include <random> // std::uniform_int_distribution için

namespace CerebrumLux { // TÜM İMPLEMENTASYON BU NAMESPACE İÇİNDE OLACAK

AIInsightsEngine::AIInsightsEngine(IntentAnalyzer& analyzer_ref, IntentLearner& learner_ref, PredictionEngine& predictor_ref,
                                 CryptofigAutoencoder& autoencoder_ref, CryptofigProcessor& cryptofig_processor_ref)
    : intent_analyzer(analyzer_ref),
      intent_learner(learner_ref),
      prediction_engine(predictor_ref),
      cryptofig_autoencoder(autoencoder_ref),
      cryptofig_processor(cryptofig_processor_ref)
{
    LOG_DEFAULT(LogLevel::INFO, "AIInsightsEngine: Initialized.");
}

// === Insight generation ===
std::vector<AIInsight> AIInsightsEngine::generate_insights(const DynamicSequence& current_sequence) {
    LOG_DEFAULT(LogLevel::DEBUG, "AIInsightsEngine::generate_insights: Yeni içgörüler uretiliyor.\n");
    std::vector<AIInsight> insights;
    auto now = std::chrono::system_clock::now();
    
    // --- Genel Performans Metriği (Grafik Besleme) - Her zaman üretilir (cooldown ile sınırlı) ---
    if (!current_sequence.latent_cryptofig_vector.empty() && !is_on_cooldown("ai_confidence_graph", std::chrono::seconds(5))) {
        float avg_latent_confidence = 0.0f;
        for (float val : current_sequence.latent_cryptofig_vector) {
            avg_latent_confidence += val;
        }
        avg_latent_confidence /= current_sequence.latent_cryptofig_vector.size();
        float normalized_confidence = std::max(0.0f, std::min(1.0f, avg_latent_confidence));

        AIInsight confidence_insight;
        confidence_insight.id = "AI_Confidence_" + std::to_string(current_sequence.timestamp_utc.time_since_epoch().count());
        confidence_insight.observation = "AI sisteminin anlık güven seviyesi: " + std::to_string(normalized_confidence);
        confidence_insight.context = "Sistem Genel Performans Metriği";
        confidence_insight.recommended_action = "Grafiği gözlemlemeye devam et.";
        confidence_insight.type = InsightType::None;
        confidence_insight.urgency = UrgencyLevel::None;
        confidence_insight.associated_cryptofig = current_sequence.latent_cryptofig_vector;
        confidence_insight.related_capsule_ids.push_back(current_sequence.id);
        insights.push_back(confidence_insight);
        insight_cooldowns["ai_confidence_graph"] = now;
        LOG_DEFAULT(LogLevel::DEBUG, "AIInsightsEngine::generate_insights: AI Güven Seviyesi içgörüsü (grafik için) üretildi: " << normalized_confidence);
    }


    // --- Çeşitli İçgörü Türleri - Her biri kendi cooldown'ı ile üretilir ve hemen eklenir ---

    AIInsight insight;

    insight = generate_reconstruction_error_insight(current_sequence);
    if (!insight.id.empty()) insights.push_back(insight);

    insight = generate_learning_rate_insight(current_sequence);
    if (!insight.id.empty()) insights.push_back(insight);

    insight = generate_system_resource_insight(current_sequence);
    if (!insight.id.empty()) insights.push_back(insight);

    insight = generate_network_activity_insight(current_sequence);
    if (!insight.id.empty()) insights.push_back(insight);

    insight = generate_application_context_insight(current_sequence);
    if (!insight.id.empty()) insights.push_back(insight);

    insight = generate_unusual_behavior_insight(current_sequence);
    if (!insight.id.empty()) insights.push_back(insight);
    
    // Eğer yukarıdaki koşulların hiçbiriyle içgörü üretilemediyse ve stabil durum içgörüsü cooldown'da değilse
    if (insights.empty() && !is_on_cooldown("stable_state", std::chrono::seconds(300))) {
        insights.push_back({"StableState_" + std::to_string(current_sequence.timestamp_utc.time_since_epoch().count()),
                            "Ic durumum stabil gorunuyor. Yeni ogrenme firsatlari icin hazirim.",
                            "Genel Durum", "Yeni özellik geliştirme veya derinlemesine öğrenme moduna geç.",
                            InsightType::None, UrgencyLevel::Low, {}, {}});
        insight_cooldowns["stable_state"] = now;
    }

    // Performans Anormalliği
    if (!sequence.get_statistical_features_vector().empty() && sequence.get_statistical_features_vector()[0] > 0.8) {
        insights.push_back({
            InsightType::PerformanceAnomaly,
            "Sistemde potansiyel performans anormalliği tespit edildi.",
            CerebrumLux::KnowledgeTopic::SystemPerformance,
            InsightSeverity::High
        });
    }

    // Öğrenme Fırsatı
    if (!sequence.get_latent_cryptofig_vector().empty() && sequence.get_latent_cryptofig_vector()[0] < 0.2) {
        insights.push_back({
            InsightType::LearningOpportunity,
            "Yeni bir öğrenme fırsatı belirlendi. Bilgi tabanının genişletilmesi önerilir.",
            CerebrumLux::KnowledgeTopic::LearningStrategy,
            InsightSeverity::Medium
        });
    }

    // Kaynak Optimizasyonu
    if (!sequence.get_statistical_features_vector().empty() && sequence.get_statistical_features_vector()[1] > 0.9) {
        insights.push_back({
            InsightType::ResourceOptimization,
            "Yüksek kaynak kullanımı tespit edildi. Optimizasyon önerileri değerlendirilmeli.",
            CerebrumLux::KnowledgeTopic::ResourceManagement,
            InsightSeverity::Medium
        });
    }
    
    // Güvenlik Uyarısı
    if (!sequence.get_statistical_features_vector().empty() && sequence.get_statistical_features_vector()[2] < 0.1) {
        insights.push_back({
            InsightType::SecurityAlert,
            "Potansiyel güvenlik açığı veya anormal davranış tespit edildi.",
            CerebrumLux::KnowledgeTopic::CyberSecurity,
            InsightSeverity::High
        });
    }

    // Kullanıcı Bağlamı
    if (!sequence.get_latent_cryptofig_vector().empty() && sequence.get_latent_cryptofig_vector()[1] > 0.7) {
        insights.push_back({
            InsightType::UserContext,
            "Kullanıcı bağlamında önemli bir değişiklik gözlemlendi. Adaptif yanıtlar için analiz ediliyor.",
            CerebrumLux::KnowledgeTopic::UserBehavior,
            InsightSeverity::Low
        });
    }

    // YENİ EKLENEN KOD: Kod Geliştirme Önerisi
    // Bu kısım, DynamicSequence'den gelen verilere dayanarak (şimdilik basitleştirilmiş bir örnekle) 
    // kod geliştirme ile ilgili içgörüler üretecektir.
    // Gelecekte, bu mantık AI'ın kod tabanı üzerindeki kendi analizlerini veya 
    // meta-evrimsel süreçlerden elde ettiği çıkarımları içerecektir.
    if (!sequence.get_statistical_features_vector().empty() && sequence.get_statistical_features_vector()[0] < 0.5 && sequence.get_statistical_features_vector()[1] < 0.5) { // Basit bir örnek koşul
        insights.push_back({
            InsightType::CodeDevelopmentSuggestion,
            "Kod tabanında potansiyel modülerlik iyileştirmeleri veya refaktör fırsatları olabilir.",
            CerebrumLux::KnowledgeTopic::CodeDevelopment,
            InsightSeverity::Medium
        });
    }

    LOG_DEFAULT(LogLevel::DEBUG, "AIInsightsEngine::generate_insights: Icgoru uretimi bitti. Sayi: " << insights.size() << "\n");

    return insights;
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
        LOG_DEFAULT(LogLevel::WARNING, "AIInsightsEngine: Reconstruction error hesaplanamadi, statistical_features boş veya boyut uyuşmuyor.");
        return 1.0f; // Yüksek bir hata değeri döndür
    }
    std::vector<float> reconstructed = this->cryptofig_autoencoder.reconstruct(statistical_features);
    return this->cryptofig_autoencoder.calculate_reconstruction_error(statistical_features, reconstructed);
}

IntentAnalyzer& AIInsightsEngine::get_analyzer() const {
    return this->intent_analyzer;
}

// === YARDIMCI İÇGÖRÜ ÜRETİM METOTLARI İMPLEMENTASYONLARI ===

AIInsight AIInsightsEngine::generate_reconstruction_error_insight(const DynamicSequence& current_sequence) {
    auto now = std::chrono::system_clock::now();
    // cooldown kontrolünü metot içinde yapıyoruz
    if (is_on_cooldown("reconstruction_error_insight", std::chrono::seconds(10))) return {};

    if (!current_sequence.statistical_features_vector.empty() && current_sequence.statistical_features_vector.size() == CryptofigAutoencoder::INPUT_DIM) {
        float reconstruction_error = calculate_autoencoder_reconstruction_error(current_sequence.statistical_features_vector);

        if (reconstruction_error > 0.1f) {
            return {"RecError_" + std::to_string(current_sequence.timestamp_utc.time_since_epoch().count()),
                    "Kriptofig analizim, mevcut sensor verisindeki bazi desenleri tam olarak ogrenemiyor. Hata: " + std::to_string(reconstruction_error),
                    "Veri Temsili", "Autoencoder ogrenme oranini artir, daha fazla epoch calistir.",
                    InsightType::PerformanceAnomaly, UrgencyLevel::High, current_sequence.latent_cryptofig_vector, {current_sequence.id}};
        } else if (reconstruction_error < 0.01f) {
            return {"RecErrorOpt_" + std::to_string(current_sequence.timestamp_utc.time_since_epoch().count()),
                    "Autoencoder'im veriyi cok iyi yeniden yapilandiriyor. Latent uzayi kucultme onerisi.",
                    "Verimlilik", "Autoencoder latent boyutunu dusurme veya budama onerisi.",
                    InsightType::EfficiencySuggestion, UrgencyLevel::Low, current_sequence.latent_cryptofig_vector, {current_sequence.id}};
        }
    } else {
        if (!is_on_cooldown("no_stats_vector", std::chrono::seconds(15))) { // Daha kısa cooldown
            return {"NoStats_" + std::to_string(current_sequence.timestamp_utc.time_since_epoch().count()),
                    "Istatistiksel ozellik vektoru bos, Autoencoder performansi hakkinda yorum yapamiyorum.",
                    "Veri Kalitesi", "Giris verisi akisini kontrol et.",
                    InsightType::PerformanceAnomaly, UrgencyLevel::Medium, {}, {current_sequence.id}};
        }
    }
    return {};
}

AIInsight AIInsightsEngine::generate_learning_rate_insight(const DynamicSequence& current_sequence) {
    auto now = std::chrono::system_clock::now();
    // cooldown kontrolünü metot içinde yapıyoruz
    if (is_on_cooldown("learning_rate_insight", std::chrono::seconds(20))) return {};

    float current_learning_rate = this->intent_learner.get_learning_rate();
    float intent_confidence = this->intent_analyzer.get_last_confidence();

    if (intent_confidence > 0.8f) { // Yüksek güven, düşük öğrenme hızı önerisi
         return {"LrnRateOpt_" + std::to_string(current_sequence.timestamp_utc.time_since_epoch().count()),
                "Niyet algilama güvenim yuksek. Ogrenme oranini optimize edebilirim.",
                "Ogrenme Stratejisi", "Ogrenme oranini azaltma veya adaptif olarak ayarlama onerisi.",
                InsightType::EfficiencySuggestion, UrgencyLevel::Low, current_sequence.latent_cryptofig_vector, {current_sequence.id}};
    } else if (intent_confidence < 0.6f) { // Düşük güven, öğrenme hızı artırma önerisi
        return {"LrnRateBoost_" + std::to_string(current_sequence.timestamp_utc.time_since_epoch().count()),
                "Niyet algilamamda güvenim düşük. Daha hizli ogrenmek icin ogrenme oranimi artirmaliyim.",
                "Ogrenme Stratejisi", "Ogrenme oranini artirma onerisi.",
                InsightType::LearningOpportunity, UrgencyLevel::High, current_sequence.latent_cryptofig_vector, {current_sequence.id}};
    }
    return {};
}

AIInsight AIInsightsEngine::generate_system_resource_insight(const DynamicSequence& current_sequence) {
    auto now = std::chrono::system_clock::now();
    // cooldown kontrolünü metot içinde yapıyoruz
    if (is_on_cooldown("system_resource_insight", std::chrono::seconds(15))) return {};

    if (current_sequence.current_cpu_usage > 80 || current_sequence.current_ram_usage > 90) {
        return {"SysResHigh_" + std::to_string(current_sequence.timestamp_utc.time_since_epoch().count()),
                "Sistem kaynaklari yÜksek kullanimda. Uygulama performansi etkilenebilir. CPU: " + std::to_string(current_sequence.current_cpu_usage) + "%, RAM: " + std::to_string(current_sequence.current_ram_usage) + "%",
                "Sistem Performansi", "Arka plan uygulamalarini kontrol et veya gereksiz isleri durdur.",
                InsightType::ResourceOptimization, UrgencyLevel::Medium, {}, {current_sequence.id}};
    } else if (current_sequence.current_cpu_usage < 20 && current_sequence.current_ram_usage < 30) {
        return {"SysResLow_" + std::to_string(current_sequence.timestamp_utc.time_since_epoch().count()),
                "Sistem kaynaklari boşta. Performansli isler icin hazir.",
                "Sistem Performansi", "Yeni görevler atayabilirsin.",
                InsightType::EfficiencySuggestion, UrgencyLevel::Low, {}, {current_sequence.id}};
    }
    return {};
}

AIInsight AIInsightsEngine::generate_network_activity_insight(const DynamicSequence& current_sequence) {
    auto now = std::chrono::system_clock::now();
    // cooldown kontrolünü metot içinde yapıyoruz
    if (is_on_cooldown("network_activity_insight", std::chrono::seconds(20))) return {};

    if (current_sequence.current_network_active && current_sequence.network_activity_level > 70) {
        return {"NetActHigh_" + std::to_string(current_sequence.timestamp_utc.time_since_epoch().count()),
                "YÜksek ağ aktivitesi tespit edildi. Protokol: " + current_sequence.network_protocol + ", Seviye: " + std::to_string(current_sequence.network_activity_level) + "%",
                "Ağ Güvenliği/Performansı", "Ağ trafiğini incele.",
                InsightType::SecurityAlert, UrgencyLevel::Medium, {}, {current_sequence.id}};
    } else if (!current_sequence.current_network_active) {
         return {"NoNet_" + std::to_string(current_sequence.timestamp_utc.time_since_epoch().count()),
                "Ağ bağlantısı yok veya çok düşük aktivite. İnternet erişimini kontrol et.",
                "Sistem Durumu", "Ağ bağlantısını kontrol etme önerisi.",
                InsightType::ResourceOptimization, UrgencyLevel::Low, {}, {current_sequence.id}};
    }
    return {};
}

AIInsight AIInsightsEngine::generate_application_context_insight(const DynamicSequence& current_sequence) {
    auto now = std::chrono::system_clock::now();
    // cooldown kontrolünü metot içinde yapıyoruz
    if (is_on_cooldown("application_context_insight", std::chrono::seconds(30))) return {};

    if (!current_sequence.current_application_context.empty() && current_sequence.current_application_context != "UnknownApp") { // Varsayılan değerden farklıysa
        return {"AppCtx_" + std::to_string(current_sequence.timestamp_utc.time_since_epoch().count()),
                "Aktif uygulama bağlamı: " + current_sequence.current_application_context,
                "Kullanıcı Bağlamı", "Kullanıcının aktif olduğu uygulamayı dikkate alarak yardımcı ol.",
                InsightType::None, UrgencyLevel::Low, {}, {current_sequence.id}};
    }
    return {};
}

AIInsight AIInsightsEngine::generate_unusual_behavior_insight(const DynamicSequence& current_sequence) {
    auto now = std::chrono::system_clock::now();
    // cooldown kontrolünü metot içinde yapıyoruz
    if (is_on_cooldown("unusual_behavior_insight", std::chrono::seconds(25))) return {};

    // %10 ihtimalle rastgele bir anormal davranış içgörüsü üret
    if (CerebrumLux::SafeRNG::get_instance().get_int(0, 100) < 10) { 
        return {"UnusualBehavior_" + std::to_string(current_sequence.timestamp_utc.time_since_epoch().count()),
                "Sistemde alışılmadık bir davranış tespit edildi. Daha detaylı analiz gerekiyor.",
                "Güvenlik", "Sistem loglarını incele.",
                InsightType::SecurityAlert, UrgencyLevel::Critical, current_sequence.latent_cryptofig_vector, {current_sequence.id}};
    }
    return {};
}

} // namespace CerebrumLux