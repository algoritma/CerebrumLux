// In main.cpp or a new test file (e.g., test_response_engine.cpp)
#include "../src/communication/response_engine.h"
#include "../src/brain/intent_analyzer.h"
#include "../src/planning_execution/goal_manager.h"
#include "../src/communication/ai_insights_engine.h"
#include "../src/data_models/dynamic_sequence.h"
#include "../src/core/enums.h" // For UserIntent, AbstractState, AIGoal
#include "../src/core/logger.h"       // LOG makrosu için
#include "../src/core/utils.h" // For convert_wstring_to_string

#include "../src/brain/autoencoder.h"           // CryptofigAutoencoder tanımı için
#include "../src/brain/cryptofig_processor.h"   // CryptofigProcessor tanımı için
#include "../src/brain/prediction_engine.h"     // PredictionEngine tanımı için
#include "../src/data_models/sequence_manager.h" // SequenceManager tanımı için

#include <iostream>
#include <vector>
#include <string>
#include <set> // For checking unique responses
#include <random> // Rastgelelik için
#include <sstream> // std::stringstream için

// Gerekli ileri bildirimler (çünkü sınıflar birbirini kullanıyor)
class DummyIntentAnalyzer;
class DummyIntentLearner;
class DummyPredictionEngine;
class DummyCryptofigAutoencoder;
class DummyCryptofigProcessor;
class DummyAIInsightsEngine;
class DummyGoalManager;

// 1. DummyIntentAnalyzer (bağımlılığı yok)
class DummyIntentAnalyzer : public IntentAnalyzer {
public:
    DummyIntentAnalyzer() : IntentAnalyzer() {}
    virtual UserIntent analyze_intent(const DynamicSequence& sequence) override { return UserIntent::None; }
};

// 2. DummyCryptofigAutoencoder (Temel sınıftan türetme ve kurucu çağırma)
class DummyCryptofigAutoencoder : public CryptofigAutoencoder {
public:
    DummyCryptofigAutoencoder() : CryptofigAutoencoder() {}
    virtual std::vector<float> encode(const std::vector<float>& input_features) const override { return std::vector<float>(CryptofigAutoencoder::LATENT_DIM, 0.0f); }
    virtual std::vector<float> decode(const std::vector<float>& latent_features) const override { return std::vector<float>(CryptofigAutoencoder::INPUT_DIM, 0.0f); }
    virtual float calculate_reconstruction_error(const std::vector<float>& original, const std::vector<float>& reconstructed) const override { return 0.0f; }
};

// 3. DummyCryptofigProcessor (Temel sınıftan türetme ve kurucu çağırma)
class DummyCryptofigProcessor : public CryptofigProcessor {
public:
    DummyCryptofigProcessor(IntentAnalyzer& analyzer_ref, CryptofigAutoencoder& autoencoder_ref)
        : CryptofigProcessor(analyzer_ref, autoencoder_ref) {}
    // get_autoencoder'ı override etmeye gerek yok, temel sınıftaki yeterli
};

// 4. DummyIntentLearner (Temel sınıftan türetme ve kurucu çağırma)
class DummyIntentLearner : public IntentLearner {
public:
    DummyIntentLearner(IntentAnalyzer& analyzer_ref) : IntentLearner(analyzer_ref) {}
    virtual void process_explicit_feedback(UserIntent predicted_intent, AIAction action, bool approved, const DynamicSequence& sequence) override {}
    virtual AbstractState infer_abstract_state(const std::deque<AtomicSignal>& recent_signals) override { return AbstractState::None; }
};

// 5. DummyPredictionEngine (Temel sınıftan türetme ve kurucu çağırma)
class DummyPredictionEngine : public PredictionEngine {
public:
    DummyPredictionEngine(IntentAnalyzer& analyzer, SequenceManager& manager) : PredictionEngine(analyzer, manager) {}
    virtual UserIntent predict_next_intent(UserIntent current_intent, const DynamicSequence& sequence) const override { return current_intent; }
};

// 6. DummyAIInsightsEngine (Temel sınıftan türetme ve kurucu çağırma)
class DummyAIInsightsEngine : public AIInsightsEngine {
public:
    DummyAIInsightsEngine(IntentAnalyzer& analyzer_ref, IntentLearner& learner_ref,
                          PredictionEngine& predictor_ref, CryptofigAutoencoder& autoencoder_ref,
                          CryptofigProcessor& cryptofig_processor_ref)
        : AIInsightsEngine(analyzer_ref, learner_ref, predictor_ref, autoencoder_ref, cryptofig_processor_ref) {}
    virtual std::vector<AIInsight> generate_insights(const DynamicSequence& sequence) override { return {}; }
    // get_analyzer'ı override etmeye gerek yok, temel sınıftaki yeterli
};

// 7. DummyGoalManager (Temel sınıftan türetme ve kurucu çağırma)
class DummyGoalManager : public GoalManager {
public:
    DummyGoalManager(AIInsightsEngine& insights_engine_ref) : GoalManager(insights_engine_ref) {}
    virtual AIGoal get_current_goal() const override { return AIGoal::None; }
};


int main() {
    LOG_INIT("test_response_engine.log");
    LOG_DEFAULT(LogLevel::INFO, "ResponseEngine Test Başladı.");

    // Bağımlılıkları main fonksiyonu içinde stack üzerinde oluştur
    DummyIntentAnalyzer dummy_analyzer;
    DummyCryptofigAutoencoder dummy_autoencoder;
    DummyCryptofigProcessor dummy_cryptofig_processor(dummy_analyzer, dummy_autoencoder);
    DummyIntentLearner dummy_learner(dummy_analyzer);
    SequenceManager dummy_sequence_manager;
    DummyPredictionEngine dummy_predictor(dummy_analyzer, dummy_sequence_manager);
    DummyAIInsightsEngine dummy_insights_engine(dummy_analyzer, dummy_learner, dummy_predictor, dummy_autoencoder, dummy_cryptofig_processor);
    DummyGoalManager dummy_goal_manager(dummy_insights_engine);

    // Test edilecek nesneyi oluştur
    ResponseEngine response_engine(dummy_analyzer, dummy_goal_manager, dummy_insights_engine);

    LOG_DEFAULT(LogLevel::INFO, "--- ResponseEngine Testleri ---");

    // --- Test Case 1: Greeting Intent (Rastgele Yanıt Seçimi) ---
    DynamicSequence seq_greeting;
    seq_greeting.latent_cryptofig_vector = {0.1f, 0.1f, 0.1f}; // Düşük aktiflik, karmaşıklık, etkileşim

    LOG_DEFAULT(LogLevel::INFO, "Test Durumu 1 (Selamlama Niyeti - Rastgele Yanıt Seçimi):");
    LOG_DEFAULT(LogLevel::INFO, "Beklenen: Farklı selamlama yanıtları (manuel gözlem)");
    std::set<std::string> greeting_responses;
    for (int i = 0; i < 5; ++i) {
        std::string response = response_engine.generate_response(UserIntent::None, AbstractState::None, AIGoal::None, seq_greeting);
        LOG_DEFAULT(LogLevel::INFO, "  Yanıt " << i + 1 << ": " << response);
        greeting_responses.insert(response);
    }
    if (greeting_responses.size() > 1) {
        LOG_DEFAULT(LogLevel::INFO, "  -> BAŞARILI: Birden fazla farklı yanıt gözlemlendi.");
    } else {
        LOG_DEFAULT(LogLevel::ERR_CRITICAL, "  -> BAŞARISIZ: Yalnızca tek bir yanıt gözlemlendi (rastgele seçim çalışmıyor olabilir).");
    }

    // --- Test Case 2: Low Productivity State ---
    DynamicSequence seq_low_perf;
    seq_low_perf.latent_cryptofig_vector = {0.5f, 0.8f, 0.4f}; // Yüksek karmaşıklık
    seq_low_perf.current_battery_percentage = 15;
    seq_low_perf.current_battery_charging = false;

    LOG_DEFAULT(LogLevel::INFO, "Test Durumu 2 (Düşük Üretkenlik Durumu):");
    LOG_DEFAULT(LogLevel::INFO, "Beklenen: 'Sistem performansınız düşük görünüyor...' veya 'Bataryanız azalıyor...' gibi bir yanıt.");
    std::string response_low_perf = response_engine.generate_response(UserIntent::None, AbstractState::LowProductivity, AIGoal::None, seq_low_perf);
    LOG_DEFAULT(LogLevel::INFO, "  Yanıt: " << response_low_perf);
    if (response_low_perf.find("Sistem performansınız düşük görünüyor") != std::string::npos ||
        response_low_perf.find("Bataryanız azalıyor") != std::string::npos) {
        LOG_DEFAULT(LogLevel::INFO, "  -> BAŞARILI: Beklenen yanıt türü alındı.");
    } else {
        LOG_DEFAULT(LogLevel::ERR_CRITICAL, "  -> BAŞARISIZ: Beklenen yanıt türü alınamadı.");
    }

    // --- Test Case 3: High Latent Complexity (Yeni Entegrasyon) ---
    DynamicSequence seq_high_complexity;
    seq_high_complexity.latent_cryptofig_vector = {0.6f, 0.9f, 0.5f}; // latent_complexity > 0.7f

    LOG_DEFAULT(LogLevel::INFO, "Test Durumu 3 (Yüksek Latent Karmaşıklık - Yeni Entegrasyon):");
    LOG_DEFAULT(LogLevel::INFO, "Beklenen: 'Bu durum oldukça karmaşık görünüyor...' ifadesini içeren bir yanıt.");
    std::string response_high_complexity = response_engine.generate_response(UserIntent::Unknown, AbstractState::None, AIGoal::None, seq_high_complexity);
    LOG_DEFAULT(LogLevel::INFO, "  Yanıt: " << response_high_complexity);
    if (response_high_complexity.find("Bu durum oldukça karmaşık görünüyor, daha fazla detaya ihtiyacım olabilir.") != std::string::npos) {
        LOG_DEFAULT(LogLevel::INFO, "  -> BAŞARILI: Yüksek latent karmaşıklık mesajı eklendi.");
    } else {
        LOG_DEFAULT(LogLevel::ERR_CRITICAL, "  -> BAŞARISIZ: Yüksek latent karmaşıklık mesajı eklenmedi.");
    }

    // --- Test Case 4: Low Latent Complexity (Yeni Entegrasyon - Mesaj Eklenmemeli) ---
    DynamicSequence seq_low_complexity;
    seq_low_complexity.latent_cryptofig_vector = {0.6f, 0.5f, 0.5f}; // latent_complexity < 0.7f

    LOG_DEFAULT(LogLevel::INFO, "Test Durumu 4 (Düşük Latent Karmaşıklık - Mesaj Eklenmemeli):");
    LOG_DEFAULT(LogLevel::INFO, "Beklenen: 'Bu durum oldukça karmaşık görünüyor...' ifadesini İÇERMEYEN bir yanıt.");
    std::string response_low_complexity = response_engine.generate_response(UserIntent::Unknown, AbstractState::None, AIGoal::None, seq_low_complexity);
    LOG_DEFAULT(LogLevel::INFO, "  Yanıt: " << response_low_complexity);
    if (response_low_complexity.find("Bu durum oldukça karmaşık görünüyor, daha fazla detaya ihtiyacım olabilir.") == std::string::npos) {
        LOG_DEFAULT(LogLevel::INFO, "  -> BAŞARILI: Düşük latent karmaşıklık mesajı eklenmedi.");
    } else {
        LOG_DEFAULT(LogLevel::ERR_CRITICAL, "  -> BAŞARISIZ: Düşük latent karmaşıklık mesajı yanlışlıkla eklendi.");
    }

    // --- Test Case 5: Low Latent Activity and Engagement (General Observation) ---
    DynamicSequence seq_low_activity_engagement;
    seq_low_activity_engagement.latent_cryptofig_vector = {0.2f, 0.4f, 0.1f}; // Düşük aktiflik ve etkileşim
    LOG_DEFAULT(LogLevel::INFO, "Test Durumu 5 (Düşük Latent Aktiflik/Etkileşim):");
    LOG_DEFAULT(LogLevel::INFO, "Beklenen: 'Latent analizim düşük aktiflik ve etkileşim görüyor...' gibi bir yanıt.");
    std::string response_low_activity_engagement = response_engine.generate_response(UserIntent::Unknown, AbstractState::None, AIGoal::None, seq_low_activity_engagement);
    LOG_DEFAULT(LogLevel::INFO, "  Yanıt: " << response_low_activity_engagement);
    if (response_low_activity_engagement.find("Latent analizim düşük aktiflik ve etkileşim görüyor") != std::string::npos) {
        LOG_DEFAULT(LogLevel::INFO, "  -> BAŞARILI: Beklenen yanıt türü alındı.");
    } else {
        LOG_DEFAULT(LogLevel::ERR_CRITICAL, "  -> BAŞARISIZ: Beklenen yanıt türü alınamadı.");
    }

    return 0;
}