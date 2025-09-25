#include "ai_insights_engine.h"
#include "../core/logger.h"
#include "../core/enums.h" // InsightType, UrgencyLevel, UserIntent, AIAction için
#include "../core/utils.h" // intent_to_string için
#include "../brain/autoencoder.h" // CryptofigAutoencoder::INPUT_DIM için

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
    
    // Simülasyon modunda değilsek veya her zaman izlememiz gereken şeyler
    // 1. Latent kriptofig verisi mevcut mu?
    if (current_sequence.latent_cryptofig_vector.empty() || current_sequence.latent_cryptofig_vector.size() != CryptofigAutoencoder::LATENT_DIM) {
        if (!is_on_cooldown("latent_data_missing", std::chrono::seconds(30))) {
            insights.push_back({"Latent kriptofig verisi eksik veya gecersiz. Niyetleri dogru analiz edemeyebilirim.", "Veri Kalitesi", "Dinamik Sekans Analizi", "Gecersiz kriptofig algilandi, daha fazla veri iste.", InsightType::PerformanceAnomaly, UrgencyLevel::High, {}, {}});
            insight_cooldowns["latent_data_missing"] = now;
        }
    }

    // 2. Niyet algılama performansı
    float intent_confidence = this->intent_analyzer.get_last_confidence();
    if (intent_confidence < 0.7f) { // Güven eşiği
        if (!is_on_cooldown("low_intent_confidence", std::chrono::seconds(60))) {
            insights.push_back({"Niyet algilama güvenim düşük. Yeni ogrenme firsatlarina ihtiyacim var.", "Niyet Algilama", "Kullanici Etkilesimi", "Ogrenme modunu tetikle, daha fazla baglamsal veri topla.", InsightType::LearningOpportunity, UrgencyLevel::Medium, {}, {}});
            insight_cooldowns["low_intent_confidence"] = now;
        }
    }
    
    // 3. Autoencoder yeniden yapılandırma hatası
    if (!current_sequence.statistical_features_vector.empty() && current_sequence.statistical_features_vector.size() == CryptofigAutoencoder::INPUT_DIM) {
        float reconstruction_error = this->cryptofig_autoencoder.calculate_reconstruction_error(
            current_sequence.statistical_features_vector,
            this->cryptofig_autoencoder.decode(this->cryptofig_autoencoder.encode(current_sequence.statistical_features_vector))
        );

        if (reconstruction_error > 0.1f) { // Yüksek hata eşiği
            if (!is_on_cooldown("high_reconstruction_error", std::chrono::seconds(90))) {
                insights.push_back({"Kriptofig analizim, mevcut sensor verisindeki bazi desenleri tam olarak ogrenemiyor. Autoencoder'in agirliklarini daha agresif ayarlamaliyim.", "Veri Temsili", "Autoencoder Performansi", "Autoencoder ogrenme oranini artir, daha fazla epoch calistir.", InsightType::PerformanceAnomaly, UrgencyLevel::High, {}, {}});
                insight_cooldowns["high_reconstruction_error"] = now;
            }
        } else if (reconstruction_error < 0.01f) { // Çok düşük hata eşiği
            if (!is_on_cooldown("low_reconstruction_error", std::chrono::seconds(120))) {
                insights.push_back({"Autoencoder'im veriyi cok iyi yeniden yapilandiriyor. Belki latent uzayi daha da kucultebilirim? Bu, verimliligi artirabilir.", "Verimlilik", "Autoencoder Optimizasyonu", "Autoencoder latent boyutunu dusurme veya budama onerisi.", InsightType::EfficiencySuggestion, UrgencyLevel::Low, {}, {}});
                insight_cooldowns["low_reconstruction_error"] = now;
            }
        }
    } else {
        if (!is_on_cooldown("no_stats_vector", std::chrono::seconds(60))) {
            insights.push_back({"Istatistiksel ozellik vektoru bos, Autoencoder performansi hakkinda yorum yapamiyorum.", "Veri Kalitesi", "Dinamik Sekans Analizi", "Giris verisi akisini kontrol et.", InsightType::PerformanceAnomaly, UrgencyLevel::Medium, {}, {}});
            insight_cooldowns["no_stats_vector"] = now;
        }
    }

    // 4. Öğrenme hızı optimizasyonu
    float current_learning_rate = this->intent_learner.get_learning_rate();
    if (current_learning_rate > 0.05f) { // Yüksek öğrenme hızı
        if (!is_on_cooldown("high_learning_rate", std::chrono::seconds(180))) {
            insights.push_back({"Ogrenme hizim su anda yuksek. Performansim stabil kalirsa biraz dusurmeye dusunebilirim.", "Ogrenme Stratejisi", "Meta-Ogrenme", "Ogrenme oranini azaltma onerisi.", InsightType::EfficiencySuggestion, UrgencyLevel::Low, {}, {}});
            insight_cooldowns["high_learning_rate"] = now;
        }
    } else if (current_learning_rate < 0.001f) { // Çok düşük öğrenme hızı
        if (!is_on_cooldown("low_learning_rate_stuck", std::chrono::seconds(240))) {
            insights.push_back({"Niyet algilamamda sorun yasiyorum ve ogrenme hizim dusuk. Daha hizli ogrenmek icin ogrenme oranimi artirmaliyim.", "Ogrenme Stratejisi", "Meta-Ogrenme", "Ogrenme oranini artirma onerisi.", InsightType::LearningOpportunity, UrgencyLevel::High, {}, {}});
            insight_cooldowns["low_learning_rate_stuck"] = now;
        }
    }

    // Ek içgörüler...

    if (insights.empty()) {
        if (!is_on_cooldown("stable_state", std::chrono::seconds(300))) {
            insights.push_back({"Ic durumum stabil gorunuyor. Yeni ogrenme firsatlari icin hazirim.", "Genel Durum", "Sistem Sağlığı", "Yeni özellik geliştirme veya derinlemesine öğrenme moduna geç.", InsightType::None, UrgencyLevel::Low, {}, {}});
            insight_cooldowns["stable_state"] = now;
        }
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

// Bu metotlar şu anda kullanılmıyor veya daha sonra doldurulacak
AIInsight AIInsightsEngine::analyze_performance(const DynamicSequence& current_sequence) {
    return {"Placeholder: Performans analizi henüz tam implemente edilmedi.", "Performance", "Generic", "N/A", InsightType::PerformanceAnomaly, UrgencyLevel::Low, {}, {}};
}

AIInsight AIInsightsEngine::identify_learning_opportunity(const DynamicSequence& current_sequence) {
    return {"Placeholder: Öğrenme fırsatları henüz tam implemente edilmedi.", "Learning", "Generic", "N/A", InsightType::LearningOpportunity, UrgencyLevel::Low, {}, {}};
}

AIInsight AIInsightsEngine::detect_anomalies(const DynamicSequence& current_sequence) {
    return {"Placeholder: Anomali tespiti henüz tam implemente edilmedi.", "Anomaly", "Generic", "N/A", InsightType::SecurityAlert, UrgencyLevel::Low, {}, {}};
}

} // namespace CerebrumLux