#include "ai_insights_engine.h" // Kendi başlık dosyasını dahil et
#include "../core/logger.h"      // LOG makrosu için
#include "../core/utils.h"       // Diğer yardımcı fonksiyonlar için
#include "../data_models/dynamic_sequence.h" // DynamicSequence için
#include "../brain/intent_analyzer.h" // IntentAnalyzer için
#include "../brain/intent_learner.h"  // IntentLearner için
#include "../brain/prediction_engine.h" // PredictionEngine için
#include "../brain/autoencoder.h"     // CryptofigAutoencoder için
#include "../brain/cryptofig_processor.h" // CryptofigProcessor için
#include <numeric>   // std::accumulate için
#include <cmath>     // std::sqrt için
#include <algorithm> // std::min/max için
#include <iostream>  // std::cout, std::cerr için
#include <iomanip>   // std::fixed, std::setprecision için
#include <sstream>   // std::stringstream için


// YENİ: AIInsightsEngine Implementasyonu
AIInsightsEngine::AIInsightsEngine(IntentAnalyzer& analyzer_ref, IntentLearner& learner_ref, 
                                 PredictionEngine& predictor_ref, CryptofigAutoencoder& autoencoder_ref,
                                 CryptofigProcessor& cryptofig_processor_ref)
    : analyzer(analyzer_ref), learner(learner_ref), predictor(predictor_ref), 
      autoencoder(autoencoder_ref), cryptofig_processor(cryptofig_processor_ref) {}

std::string AIInsightsEngine::generateResponse(UserIntent intent, const std::vector<float>& latent_cryptofig_vector) {
    // ... (Implementasyon)
    return "Response"; // Örnek bir yanıt
}

float AIInsightsEngine::calculate_average_feedback_score(UserIntent intent_id) const {
    auto it = learner.get_implicit_feedback_history().find(intent_id);
    if (it != learner.get_implicit_feedback_history().end() && !it->second.empty()) {
        return std::accumulate(it->second.begin(), it->second.end(), 0.0f) / it->second.size();
    }
    return 0.0f;
}

float AIInsightsEngine::calculate_autoencoder_reconstruction_error(const std::vector<float>& statistical_features) const {
    if (statistical_features.empty() || statistical_features.size() != CryptofigAutoencoder::INPUT_DIM) {
        LOG_DEFAULT(LogLevel::ERR_CRITICAL, "AIInsightsEngine::calculate_autoencoder_reconstruction_error: İstatistiksel özellik vektörü boş veya boyut uyuşmuyor. Yüksek hata döndürülüyor.\n");
        return 1.0f; // Hata durumunda yüksek hata döndür
    }
    std::vector<float> reconstructed = autoencoder.reconstruct(statistical_features);
    float error = 0.0f;
    for (size_t i = 0; i < statistical_features.size(); ++i) {
        error += (statistical_features[i] - reconstructed[i]) * (statistical_features[i] - reconstructed[i]);
    }
    return std::sqrt(error / statistical_features.size()); // RMSE
}

// IntentAnalyzer üyesine erişim için getter metodunun implementasyonu
IntentAnalyzer& AIInsightsEngine::get_analyzer() const {
    return analyzer;
}


std::vector<AIInsight> AIInsightsEngine::generate_insights(const DynamicSequence& current_sequence) {
    LOG_DEFAULT(LogLevel::DEBUG, "AIInsightsEngine::generate_insights: Icgoru uretimi basladi.\n");
    std::vector<AIInsight> insights;
    auto now = std::chrono::steady_clock::now();

    auto is_on_cooldown = [&](const std::string& key, int seconds) -> bool {
        auto it = insight_cooldowns.find(key);
        if (it != insight_cooldowns.end()) {
            auto time_since_last = std::chrono::duration_cast<std::chrono::seconds>(now - it->second).count();
            if (time_since_last < seconds) {
                return true;
            }
        }
        return false;
    };

    // 1. Niyet Analizörü Performansı Hakkında İçgörüler
    if (current_sequence.latent_cryptofig_vector.empty() || current_sequence.latent_cryptofig_vector.size() != CryptofigAutoencoder::LATENT_DIM) {
        if (!is_on_cooldown("latent_data_missing", 300)) {
            insights.push_back({"Latent kriptofig verisi eksik veya geçersiz. Niyetleri doğru analiz edemeyebilirim.", AIAction::SuggestSelfImprovement, 1.0f});
            insight_cooldowns["latent_data_missing"] = now;
        }
    } else {
        UserIntent current_predicted_intent = analyzer.analyze_intent(current_sequence);
        if (current_predicted_intent == UserIntent::Unknown) {
            if (!is_on_cooldown("unknown_intent_struggle", 120)) {
                insights.push_back({"Şu anki niyetinizi tam olarak algılamakta zorlanıyorum. Yeni öğrenme fırsatlarına ihtiyacım var.", AIAction::SuggestSelfImprovement, 0.8f});
                insight_cooldowns["unknown_intent_struggle"] = now;
            }
        }
        
        float fast_typing_feedback = calculate_average_feedback_score(UserIntent::FastTyping);
        if (fast_typing_feedback < -0.2f && learner.get_implicit_feedback_history().count(UserIntent::FastTyping) && learner.get_implicit_feedback_history().at(UserIntent::FastTyping).size() > learner.get_feedback_history_size() / 2) { 
            if (!is_on_cooldown("low_fast_typing_feedback", 180)) {
                insights.push_back({"Hızlı yazım modunda kullanıcı geri bildirimlerim düşük seyrediyor. Bu niyet için şablon ağırlıklarımı gözden geçirmeliyim.", AIAction::SuggestSelfImprovement, 0.7f});
                insight_cooldowns["low_fast_typing_feedback"] = now;
            }
        }
    }

    // 2. Autoencoder Performansı Hakkında İçgörüler
    if (!current_sequence.statistical_features_vector.empty()) {
        float reconstruction_error = calculate_autoencoder_reconstruction_error(current_sequence.statistical_features_vector);
        if (reconstruction_error > 0.3f) { 
            if (!is_on_cooldown("high_reconstruction_error", 60)) {
                insights.push_back({"Kriptofig analizim, mevcut sensör verisindeki bazı desenleri tam olarak öğrenemiyor. Autoencoder'ın ağırlıklarını daha agresif ayarlamalıyım.", AIAction::SuggestSelfImprovement, 0.9f});
                insight_cooldowns["high_reconstruction_error"] = now;
            }
        } else if (reconstruction_error < 0.05f) { 
            if (!is_on_cooldown("low_reconstruction_error", 300)) {
                insights.push_back({"Autoencoder'ım veriyi çok iyi yeniden yapılandırıyor. Belki latent uzayı daha da küçültebilirim? Bu, verimliliği artırabilir.", AIAction::SuggestSelfImprovement, 0.2f});
                insight_cooldowns["low_reconstruction_error"] = now;
            }
        }
    } else {
        if (!is_on_cooldown("no_stats_vector", 300)) {
             insights.push_back({"İstatistiksel özellik vektörü boş, Autoencoder performansı hakkında yorum yapamıyorum.", AIAction::None, 0.0f});
             insight_cooldowns["no_stats_vector"] = now;
        }
    }

    // 3. Genel Öğrenme Mekanizması Hakkında İçgörüler (Meta-Ayarlama)
    if (learner.get_learning_rate() > 0.05f && learner.get_implicit_feedback_history().size() > learner.get_feedback_history_size() / 2) { 
        if (!is_on_cooldown("high_learning_rate", 180)) {
            insights.push_back({"Öğrenme hızım şu anda yüksek. Performansım stabil kalırsa biraz düşürmeyi düşünebilirim.", AIAction::SuggestSelfImprovement, 0.3f});
            insight_cooldowns["high_learning_rate"] = now;
        }
    } else if (learner.get_learning_rate() < 0.005f && learner.get_implicit_feedback_history().size() > learner.get_feedback_history_size() / 2 && calculate_average_feedback_score(UserIntent::Unknown) < -0.5f) { 
        if (!is_on_cooldown("low_learning_rate_stuck", 120)) {
            insights.push_back({"Niyet algılamamda sorun yaşıyorum ve öğrenme hızım düşük. Daha hızlı öğrenmek için öğrenme oranımı artırmalıyım.", AIAction::SuggestSelfImprovement, 0.9f});
            insight_cooldowns["low_learning_rate_stuck"] = now;
        }
    }
    
    if (insights.empty()) {
        if (!is_on_cooldown("stable_state", 60)) {
            insights.push_back({"İç durumum stabil görünüyor. Yeni öğrenme fırsatları için hazırım.", AIAction::None, 0.0f});
            insight_cooldowns["stable_state"] = now;
        }
    }

    LOG_DEFAULT(LogLevel::DEBUG, "AIInsightsEngine::generate_insights: Icgoru uretimi bitti. Sayi: " << insights.size() << "\n");
    return insights;
}