#include "ai_insights_engine.h"
#include "../core/logger.h"
#include "../core/enums.h" // InsightType, UrgencyLevel, UserIntent, AIAction, KnowledgeTopic, InsightSeverity için
#include "../core/utils.h" // intent_to_string, SafeRNG için
#include "../brain/autoencoder.h" // CryptofigAutoencoder::INPUT_DIM için
#include "../data_models/dynamic_sequence.h" // DynamicSequence için
#include <numeric> // std::accumulate için
#include <algorithm> // std::min, std::max için
#include <random> // std::uniform_int_distribution, std::normal_distribution için
 

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
}

// === Insight generation ===
std::vector<AIInsight> AIInsightsEngine::generate_insights(const DynamicSequence& current_sequence) {
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "AIInsightsEngine::generate_insights: Yeni içgörüler uretiliyor.\n");
    std::vector<AIInsight> insights;
    auto now = std::chrono::system_clock::now();

    // YENİ KOD: Kod Karmaşıklığı Simülasyonu
    // AI'ın kendi içsel kod tabanı veya dinamik süreçlerinden gelen metrikleri simüle ediyoruz.
    // Her döngüde karmaşıklığı hafifçe değiştir.
    if (!is_on_cooldown("code_complexity_simulation", std::chrono::seconds(10))) { // Her 10 saniyede bir güncelle
        std::random_device rd;
        std::mt19937 gen(rd());
        std::normal_distribution<float> d(0.0f, 0.05f); // Ortalama 0, standart sapma 0.05
        
        last_simulated_code_complexity += d(gen);
        // Karmaşıklık değerini 0.0 ile 1.0 arasında tut
        last_simulated_code_complexity = std::max(0.0f, std::min(1.0f, last_simulated_code_complexity));
        LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "AIInsightsEngine: Simüle Kod Karmaşıklığı Güncellendi: " << last_simulated_code_complexity);
        insight_cooldowns["code_complexity_simulation"] = now;
    }
    // END YENİ KOD: Kod Karmaşıklığı Simülasyonu

    // YENİ KOD: Kod Okunabilirlik Skoru Simülasyonu
    if (!is_on_cooldown("code_readability_simulation", std::chrono::seconds(15))) { // Her 15 saniyede bir güncelle
        std::random_device rd;
        std::mt19937 gen(rd());
        std::normal_distribution<float> d(0.0f, 0.08f); // Ortalama 0, standart sapma 0.08
        
        last_simulated_code_readability += d(gen);
        // Okunabilirlik değerini 0.0 ile 1.0 arasında tut
        last_simulated_code_readability = std::max(0.0f, std::min(1.0f, last_simulated_code_readability));
        LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "AIInsightsEngine: Simüle Kod Okunabilirlik Skoru Güncellendi: " << last_simulated_code_readability);
        insight_cooldowns["code_readability_simulation"] = now;
    }
    // END YENİ KOD: Kod Okunabilirlik Skoru Simülasyonu

    // YENİ KOD: Kod Optimizasyon Potansiyeli Simülasyonu
    if (!is_on_cooldown("code_optimization_potential_simulation", std::chrono::seconds(20))) { // Her 20 saniyede bir güncelle
        std::random_device rd;
        std::mt19937 gen(rd());
        std::normal_distribution<float> d(0.0f, 0.07f); // Ortalama 0, standart sapma 0.07
        
        last_simulated_optimization_potential += d(gen);
        // Optimizasyon Potansiyeli değerini 0.0 ile 1.0 arasında tut
        last_simulated_optimization_potential = std::max(0.0f, std::min(1.0f, last_simulated_optimization_potential));
        LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "AIInsightsEngine: Simüle Kod Optimizasyon Potansiyeli Güncellendi: " << last_simulated_optimization_potential);
        insight_cooldowns["code_optimization_potential_simulation"] = now;
    }
    // END YENİ KOD: Kod Optimizasyon Potansiyeli Simülasyonu

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
        confidence_insight.type = CerebrumLux::InsightType::None; // Veya uygun bir tip
        confidence_insight.urgency = CerebrumLux::UrgencyLevel::None; // Veya uygun bir seviye
        confidence_insight.associated_cryptofig = current_sequence.latent_cryptofig_vector;
        confidence_insight.related_capsule_ids.push_back(current_sequence.id);
        insights.push_back(confidence_insight);
        insight_cooldowns["ai_confidence_graph"] = now;
        LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "AIInsightsEngine::generate_insights: AI Güven Seviyesi içgörüsü (grafik için) üretildi: " << normalized_confidence);
    }


    // --- Çeşitli İçgörü Türleri - Her biri kendi cooldown'ı ile üretilir ve hemen eklenir ---

    AIInsight insight_from_helper; // Geçici bir AIInsight objesi

    insight_from_helper = generate_reconstruction_error_insight(current_sequence);
    if (!insight_from_helper.id.empty()) insights.push_back(insight_from_helper);

    insight_from_helper = generate_learning_rate_insight(current_sequence);
    if (!insight_from_helper.id.empty()) insights.push_back(insight_from_helper);

    insight_from_helper = generate_system_resource_insight(current_sequence);
    if (!insight_from_helper.id.empty()) insights.push_back(insight_from_helper);

    insight_from_helper = generate_network_activity_insight(current_sequence);
    if (!insight_from_helper.id.empty()) insights.push_back(insight_from_helper);

    insight_from_helper = generate_application_context_insight(current_sequence);
    if (!insight_from_helper.id.empty()) insights.push_back(insight_from_helper);

    insight_from_helper = generate_unusual_behavior_insight(current_sequence);
    if (!insight_from_helper.id.empty()) insights.push_back(insight_from_helper);

    // Eğer yukarıdaki koşulların hiçbiriyle içgörü üretilemediyse ve stabil durum içgörüsü cooldown'da değilse
    if (insights.empty() && !is_on_cooldown("stable_state", std::chrono::seconds(300))) {
        insights.push_back({"StableState_" + std::to_string(current_sequence.timestamp_utc.time_since_epoch().count()),
                            "İç durumum stabil görünüyor. Yeni öğrenme fırsatları için hazırım.",
                            "Genel Durum", "Yeni özellik geliştirme veya derinlemesine öğrenme moduna geç.",
                            CerebrumLux::InsightType::None, CerebrumLux::UrgencyLevel::Low, {}, {}});
        insight_cooldowns["stable_state"] = now;
    }

    // Performans Anormalliği
    if (!current_sequence.statistical_features_vector.empty() && current_sequence.statistical_features_vector[0] > 0.8) {
        insights.push_back({
            "PerformanceAnomaly_" + std::to_string(now.time_since_epoch().count()), // id
            "Sistemde potansiyel performans anormalliği tespit edildi.",             // observation
            knowledge_topic_to_string(CerebrumLux::KnowledgeTopic::SystemPerformance), // context (string'e çevrildi)
            "Performans izleme araçlarını kontrol edin ve anormal süreçleri belirleyin.", // recommended_action
            CerebrumLux::InsightType::PerformanceAnomaly,                           // type
            insight_severity_to_urgency_level(CerebrumLux::InsightSeverity::High),  // urgency (UrgencyLevel'a çevrildi)
            current_sequence.latent_cryptofig_vector,                               // associated_cryptofig
            {current_sequence.id}                                                   // related_capsule_ids
        });
    }

    // Öğrenme Fırsatı
    if (!current_sequence.latent_cryptofig_vector.empty() && current_sequence.latent_cryptofig_vector[0] < 0.2) {
        insights.push_back({
            "LearningOpportunity_" + std::to_string(now.time_since_epoch().count()), // id
            "Yeni bir öğrenme fırsatı belirlendi. Bilgi tabanının genişletilmesi önerilir.", // observation
            knowledge_topic_to_string(CerebrumLux::KnowledgeTopic::LearningStrategy), // context (string'e çevrildi)
            "Mevcut bilgi tabanını gözden geçirin ve yeni öğrenme kaynakları arayın.", // recommended_action
            CerebrumLux::InsightType::LearningOpportunity,                          // type
            insight_severity_to_urgency_level(CerebrumLux::InsightSeverity::Medium),// urgency (UrgencyLevel'a çevrildi)
            current_sequence.latent_cryptofig_vector,                               // associated_cryptofig
            {current_sequence.id}                                                   // related_capsule_ids
        });
    }

    // Kaynak Optimizasyonu
    if (!current_sequence.statistical_features_vector.empty() && current_sequence.statistical_features_vector[1] > 0.9) {
        insights.push_back({
            "ResourceOptimization_" + std::to_string(now.time_since_epoch().count()), // id
            "Yüksek kaynak kullanımı tespit edildi. Optimizasyon önerileri değerlendirilmeli.", // observation
            knowledge_topic_to_string(CerebrumLux::KnowledgeTopic::ResourceManagement), // context (string'e çevrildi)
            "Arka plan uygulamalarını kontrol edin veya gereksiz işlemleri durdurun.", // recommended_action
            CerebrumLux::InsightType::ResourceOptimization,                         // type
            insight_severity_to_urgency_level(CerebrumLux::InsightSeverity::Medium),// urgency (UrgencyLevel'a çevrildi)
            {},                                                                     // associated_cryptofig (bu içgörü için gerekli olmayabilir)
            {current_sequence.id}                                                   // related_capsule_ids
        });
    }

    // Güvenlik Uyarısı
    if (!current_sequence.statistical_features_vector.empty() && current_sequence.statistical_features_vector[2] < 0.1) {
        insights.push_back({
            "SecurityAlert_" + std::to_string(now.time_since_epoch().count()),     // id
            "Potansiyel güvenlik açığı veya anormal davranış tespit edildi.",        // observation
            knowledge_topic_to_string(CerebrumLux::KnowledgeTopic::CyberSecurity),  // context (string'e çevrildi)
            "Sistem loglarını inceleyin ve güvenlik taraması yapın.",               // recommended_action
            CerebrumLux::InsightType::SecurityAlert,                                // type
            insight_severity_to_urgency_level(CerebrumLux::InsightSeverity::High),  // urgency (UrgencyLevel'a çevrildi)
            current_sequence.latent_cryptofig_vector,                               // associated_cryptofig
            {current_sequence.id}                                                   // related_capsule_ids
        });
    }

    // Kullanıcı Bağlamı
    if (!current_sequence.latent_cryptofig_vector.empty() && current_sequence.latent_cryptofig_vector[1] > 0.7) {
        insights.push_back({
            "UserContext_" + std::to_string(now.time_since_epoch().count()),       // id
            "Kullanıcı bağlamında önemli bir değişiklik gözlemlendi. Adaptif yanıtlar için analiz ediliyor.", // observation
            knowledge_topic_to_string(CerebrumLux::KnowledgeTopic::UserBehavior),   // context (string'e çevrildi)
            "Kullanıcının mevcut aktivitesine göre adaptif yanıtlar hazırlayın.",    // recommended_action
            CerebrumLux::InsightType::UserContext,                                  // type
            insight_severity_to_urgency_level(CerebrumLux::InsightSeverity::Low),   // urgency (UrgencyLevel'a çevrildi)
            current_sequence.latent_cryptofig_vector,                               // associated_cryptofig
            {current_sequence.id}                                                   // related_capsule_ids
        });
    }

    // YENİ EKLENEN KOD: Daha Spesifik Kod Geliştirme Önerileri (Çeşitli simüle metrikler bazında)
    // 1. Yüksek Karmaşıklık ve Düşük Okunabilirlik
    if (last_simulated_code_complexity > 0.8f && last_simulated_code_readability < 0.4f && !is_on_cooldown("high_complexity_low_readability_suggestion", std::chrono::seconds(60))) {
        insight_cooldowns["high_complexity_low_readability_suggestion"] = now;
        insights.push_back({
            "CodeDev_HighComplexityLowReadability_" + std::to_string(now.time_since_epoch().count()),
            "Kritik seviyede yüksek kod karmaşıklığı (" + std::to_string(last_simulated_code_complexity) + ") ve düşük okunabilirlik (" + std::to_string(last_simulated_code_readability) + ") tespit edildi. Modülerlik iyileştirmeleri ve refaktör ACİL!",
            knowledge_topic_to_string(CerebrumLux::KnowledgeTopic::CodeDevelopment),
            "Kritik karmaşıklığa sahip fonksiyonları/sınıfları belirleyin, sorumluğu tek olan parçalara bölün ve kod yorumlamasını artırın. İsimlendirme standartlarını gözden geçirin.",
            CerebrumLux::InsightType::CodeDevelopmentSuggestion,
            insight_severity_to_urgency_level(CerebrumLux::InsightSeverity::Critical),
            current_sequence.latent_cryptofig_vector,
            {current_sequence.id}
        });
    }
    // 2. Yüksek Optimizasyon Potansiyeli
    else if (last_simulated_optimization_potential > 0.7f && !is_on_cooldown("high_optimization_potential_suggestion", std::chrono::seconds(90))) {
        insight_cooldowns["high_optimization_potential_suggestion"] = now;
        insights.push_back({
            "CodeDev_HighOptimizationPotential_" + std::to_string(now.time_since_epoch().count()),
            "Yüksek performans optimizasyon potansiyeli tespit edildi (" + std::to_string(last_simulated_optimization_potential) + "). Bazı döngüler veya algoritmalar iyileştirilebilir.",
            knowledge_topic_to_string(CerebrumLux::KnowledgeTopic::CodeDevelopment),
            "Sıkça çağrılan ve zaman alan kod bloklarını profilleyin. Alternatif veri yapıları veya algoritmalar kullanarak performansı artırın.",
            CerebrumLux::InsightType::CodeDevelopmentSuggestion,
            insight_severity_to_urgency_level(CerebrumLux::InsightSeverity::High),
            current_sequence.latent_cryptofig_vector,
            {current_sequence.id}
        });
    }
    // 3. Genel Modülerlik/Refaktör Fırsatları (Orta Karmaşıklık/Okunabilirlik)
    else if (last_simulated_code_complexity > 0.6f && last_simulated_code_readability < 0.6f && !is_on_cooldown("medium_complexity_readability_suggestion", std::chrono::seconds(120))) {
        insight_cooldowns["medium_complexity_readability_suggestion"] = now;
        insights.push_back({
            "CodeDev_MediumComplexityReadability_" + std::to_string(now.time_since_epoch().count()),
            "Kod tabanında potansiyel modülerlik iyileştirmeleri veya refaktör fırsatları olabilir (Karmaşıklık: " + std::to_string(last_simulated_code_complexity) + ", Okunabilirlik: " + std::to_string(last_simulated_code_readability) + ").",
            knowledge_topic_to_string(CerebrumLux::KnowledgeTopic::CodeDevelopment),
            "Kod tabanını analiz ederek potansiyel modülerlik veya refaktör alanlarını belirleyin. Benzer işlevselliğe sahip parçaları soyutlayın.",
            CerebrumLux::InsightType::CodeDevelopmentSuggestion,
            insight_severity_to_urgency_level(CerebrumLux::InsightSeverity::Medium),
            current_sequence.latent_cryptofig_vector,
            {current_sequence.id}
        });
    }
    // 4. İyi Durumda Ama Küçük İyileştirmeler (Düşük Karmaşıklık, Yüksek Okunabilirlik)
    else if (last_simulated_code_complexity < 0.3f && last_simulated_code_readability > 0.8f && last_simulated_optimization_potential < 0.2f && !is_on_cooldown("minor_code_improvement_suggestion", std::chrono::seconds(180))) {
        insight_cooldowns["minor_code_improvement_suggestion"] = now;
        insights.push_back({
            "CodeDevSuggest_HighComplexity_" + std::to_string(now.time_since_epoch().count()), // id
            "Mevcut kod tabanı iyi durumda. Ancak küçük çaplı stil veya dokümantasyon iyileştirmeleri yapılabilir (Karmaşıklık: " + std::to_string(last_simulated_code_complexity) + ", Okunabilirlik: " + std::to_string(last_simulated_code_readability) + ").", // observation

            knowledge_topic_to_string(CerebrumLux::KnowledgeTopic::CodeDevelopment),// context (string'e çevrildi)
            "Kodlama stil rehberini gözden geçirin ve tutarlılığı artırın. Eksik dokümantasyonu tamamlayın veya mevcut yorumları geliştirin.", // recommended_action
            CerebrumLux::InsightType::CodeDevelopmentSuggestion,                    // type

            insight_severity_to_urgency_level(CerebrumLux::InsightSeverity::Critical),// urgency (UrgencyLevel'a çevrildi)
            current_sequence.latent_cryptofig_vector,                               // associated_cryptofig
            {current_sequence.id}                                                   // related_capsule_ids
        });
    }

    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "AIInsightsEngine::generate_insights: Icgoru uretimi bitti. Sayi: " << insights.size() << "\n");

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
        LOG_DEFAULT(CerebrumLux::LogLevel::WARNING, "AIInsightsEngine: Reconstruction error hesaplanamadi, statistical_features boş veya boyut uyuşmuyor.");
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
            this->insight_cooldowns["reconstruction_error_insight"] = now; // Insight üretildiğinde cooldown'a al
            return {"RecError_" + std::to_string(current_sequence.timestamp_utc.time_since_epoch().count()),
                    "Kriptofig analizim, mevcut sensor verisindeki bazi desenleri tam olarak ogrenemiyor. Hata: " + std::to_string(reconstruction_error),
                    "Veri Temsili", "Autoencoder ogrenme oranini artir, daha fazla epoch calistir.",
                    CerebrumLux::InsightType::PerformanceAnomaly, CerebrumLux::UrgencyLevel::High, current_sequence.latent_cryptofig_vector, {current_sequence.id}};
        } else if (reconstruction_error < 0.01f) {
            this->insight_cooldowns["reconstruction_error_insight"] = now; // Insight üretildiğinde cooldown'a al
            return {"RecErrorOpt_" + std::to_string(current_sequence.timestamp_utc.time_since_epoch().count()),
                    "Autoencoder'im veriyi cok iyi yeniden yapilandiriyor. Latent uzayi kucultme onerisi.",
                    "Verimlilik", "Autoencoder latent boyutunu dusurme veya budama onerisi.",
                    CerebrumLux::InsightType::EfficiencySuggestion, CerebrumLux::UrgencyLevel::Low, current_sequence.latent_cryptofig_vector, {current_sequence.id}};
        }
    } else {
        if (!is_on_cooldown("no_stats_vector", std::chrono::seconds(15))) {
            this->insight_cooldowns["no_stats_vector"] = now; // Insight üretildiğinde cooldown'a al
            return {"NoStats_" + std::to_string(current_sequence.timestamp_utc.time_since_epoch().count()),
                    "Istatistiksel ozellik vektoru bos, Autoencoder performansi hakkinda yorum yapamiyorum.",
                    "Veri Kalitesi", "Giris verisi akisini kontrol et.",
                    CerebrumLux::InsightType::PerformanceAnomaly, CerebrumLux::UrgencyLevel::Medium, {}, {current_sequence.id}};
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
         this->insight_cooldowns["learning_rate_insight"] = now; // Insight üretildiğinde cooldown'a al
         return {"LrnRateOpt_" + std::to_string(current_sequence.timestamp_utc.time_since_epoch().count()),
                "Niyet algilama güvenim yuksek. Ogrenme oranini optimize edebilirim.",
                "Ogrenme Stratejisi", "Ogrenme oranini azaltma veya adaptif olarak ayarlama onerisi.",
                CerebrumLux::InsightType::EfficiencySuggestion, CerebrumLux::UrgencyLevel::Low, current_sequence.latent_cryptofig_vector, {current_sequence.id}};
    } else if (intent_confidence < 0.6f) { // Düşük güven, öğrenme hızı artırma önerisi
        this->insight_cooldowns["learning_rate_insight"] = now; // Insight üretildiğinde cooldown'a al
        return {"LrnRateBoost_" + std::to_string(current_sequence.timestamp_utc.time_since_epoch().count()),
                "Niyet algilamamda güvenim düşük. Daha hizli ogrenmek icin ogrenme oranimi artirmaliyim.",
                "Ogrenme Stratejisi", "Ogrenme oranini artirma onerisi.",
                CerebrumLux::InsightType::LearningOpportunity, CerebrumLux::UrgencyLevel::High, current_sequence.latent_cryptofig_vector, {current_sequence.id}};
    }
    return {};
}

AIInsight AIInsightsEngine::generate_system_resource_insight(const DynamicSequence& current_sequence) {
    auto now = std::chrono::system_clock::now();
    // cooldown kontrolünü metot içinde yapıyoruz
    if (is_on_cooldown("system_resource_insight", std::chrono::seconds(15))) return {};

    if (current_sequence.current_cpu_usage > 80 || current_sequence.current_ram_usage > 90) {
        this->insight_cooldowns["system_resource_insight"] = now; // Insight üretildiğinde cooldown'a al
        return {"SysResHigh_" + std::to_string(current_sequence.timestamp_utc.time_since_epoch().count()),
                "Sistem kaynaklari yÜksek kullanimda. Uygulama performansi etkilenebilir. CPU: " + std::to_string(current_sequence.current_cpu_usage) + "%, RAM: " + std::to_string(current_sequence.current_ram_usage) + "%",
                "Sistem Performansi", "Arka plan uygulamalarini kontrol et veya gereksiz isleri durdur.",
                CerebrumLux::InsightType::ResourceOptimization, CerebrumLux::UrgencyLevel::Medium, {}, {current_sequence.id}};
    } else if (current_sequence.current_cpu_usage < 20 && current_sequence.current_ram_usage < 30) {
        this->insight_cooldowns["system_resource_insight"] = now; // Insight üretildiğinde cooldown'a al
        return {"SysResLow_" + std::to_string(current_sequence.timestamp_utc.time_since_epoch().count()),
                "Sistem kaynaklari boşta. Performansli isler icin hazir.",
                "Sistem Performansi", "Yeni görevler atayabilirsin.",
                CerebrumLux::InsightType::EfficiencySuggestion, CerebrumLux::UrgencyLevel::Low, {}, {current_sequence.id}};
    }
    return {};
}

AIInsight AIInsightsEngine::generate_network_activity_insight(const DynamicSequence& current_sequence) {
    auto now = std::chrono::system_clock::now();
    // cooldown kontrolünü metot içinde yapıyoruz
    if (is_on_cooldown("network_activity_insight", std::chrono::seconds(20))) return {};

    if (current_sequence.current_network_active && current_sequence.network_activity_level > 70) {
        this->insight_cooldowns["network_activity_insight"] = now; // Insight üretildiğinde cooldown'a al
        return {"NetActHigh_" + std::to_string(current_sequence.timestamp_utc.time_since_epoch().count()),
                "YÜksek ağ aktivitesi tespit edildi. Protokol: " + current_sequence.network_protocol + ", Seviye: " + std::to_string(current_sequence.network_activity_level) + "%",
                "Ağ Güvenliği/Performansı", "Ağ trafiğini incele.",
                CerebrumLux::InsightType::SecurityAlert, CerebrumLux::UrgencyLevel::Medium, {}, {current_sequence.id}};
    } else if (!current_sequence.current_network_active) {
         this->insight_cooldowns["network_activity_insight"] = now; // Insight üretildiğinde cooldown'a al
         return {"NoNet_" + std::to_string(current_sequence.timestamp_utc.time_since_epoch().count()),
                "Ağ bağlantısı yok veya çok düşük aktivite. İnternet erişimini kontrol et.",
                "Sistem Durumu", "Ağ bağlantısını kontrol etme önerisi.",
                CerebrumLux::InsightType::ResourceOptimization, CerebrumLux::UrgencyLevel::Low, {}, {current_sequence.id}};
    }
    return {};
}

AIInsight AIInsightsEngine::generate_application_context_insight(const DynamicSequence& current_sequence) {
    auto now = std::chrono::system_clock::now();
    // cooldown kontrolünü metot içinde yapıyoruz
    if (is_on_cooldown("application_context_insight", std::chrono::seconds(30))) return {};

    if (!current_sequence.current_application_context.empty() && current_sequence.current_application_context != "UnknownApp") {
        this->insight_cooldowns["application_context_insight"] = now; // Insight üretildiğinde cooldown'a al
        return {"AppCtx_" + std::to_string(current_sequence.timestamp_utc.time_since_epoch().count()),
                "Aktif uygulama bağlamı: " + current_sequence.current_application_context,
                "Kullanıcı Bağlamı", "Kullanıcının aktif olduğu uygulamayı dikkate alarak yardımcı ol.",
                CerebrumLux::InsightType::None, CerebrumLux::UrgencyLevel::Low, {}, {current_sequence.id}};
    }
    return {};
}

AIInsight AIInsightsEngine::generate_unusual_behavior_insight(const DynamicSequence& current_sequence) {
    auto now = std::chrono::system_clock::now();
    // cooldown kontrolünü metot içinde yapıyoruz
    if (is_on_cooldown("unusual_behavior_insight", std::chrono::seconds(25))) return {};

    if (CerebrumLux::SafeRNG::get_instance().get_int(0, 100) < 10) {
        this->insight_cooldowns["unusual_behavior_insight"] = now; // Insight üretildiğinde cooldown'a al
        return {"UnusualBehavior_" + std::to_string(current_sequence.timestamp_utc.time_since_epoch().count()),
                "Sistemde alışılmadık bir davranış tespit edildi. Daha detaylı analiz gerekiyor.",
                "Güvenlik", "Sistem loglarını incele.",
                CerebrumLux::InsightType::SecurityAlert, CerebrumLux::UrgencyLevel::Critical, current_sequence.latent_cryptofig_vector, {current_sequence.id}};
    }
    return {};
}

} // namespace CerebrumLux