// In main.cpp or a new test file (e.g., test_response_engine.cpp)
#include "../src/communication/response_engine.h"
#include "../src/brain/intent_analyzer.h"
#include "../src/planning_execution/goal_manager.h"
#include "../src/communication/ai_insights_engine.h"
#include "../src/data_models/dynamic_sequence.h"
#include "../src/core/enums.h" // For UserIntent, AbstractState, AIGoal
#include "../src/core/logger.h"       // LOG makrosu için (varsa)

#include "../src/brain/autoencoder.h"           // CryptofigAutoencoder tanımı için
#include "../src/brain/cryptofig_processor.h"   // CryptofigProcessor tanımı için
#include "../src/brain/prediction_engine.h"     // PredictionEngine tanımı için
#include "../src/data_models/sequence_manager.h" // SequenceManager tanımı için

#include <iostream>
#include <vector>
#include <string>
#include <set> // For checking unique responses
#include <random> // Rastgelelik için

// Gerekli ileri bildirimler (çünkü sınıflar birbirini kullanıyor)
class DummyIntentAnalyzer;
class DummyIntentLearner;
class DummyPredictionEngine;
class DummyCryptofigAutoencoder;
class DummyCryptofigProcessor;
class DummyAIInsightsEngine;
class DummyGoalManager;
// SequenceManager'ın kendisi doğrudan kullanılacağı için ileri bildirime gerek yok.


// 1. DummyIntentAnalyzer (bağımlılığı yok)
class DummyIntentAnalyzer : public IntentAnalyzer {
public:
    DummyIntentAnalyzer() : IntentAnalyzer() {}
    virtual UserIntent analyze_intent(const DynamicSequence& sequence) override { return UserIntent::None; }
};
static DummyIntentAnalyzer dummy_analyzer;

// 2. DummyCryptofigAutoencoder (Temel sınıftan türetme ve kurucu çağırma)
class DummyCryptofigAutoencoder : public CryptofigAutoencoder {
public:
    DummyCryptofigAutoencoder() : CryptofigAutoencoder() {}
    virtual std::vector<float> encode(const std::vector<float>& input_features) const override { return std::vector<float>(CryptofigAutoencoder::LATENT_DIM, 0.0f); }
    virtual std::vector<float> decode(const std::vector<float>& latent_features) const override { return std::vector<float>(CryptofigAutoencoder::INPUT_DIM, 0.0f); }
    virtual float calculate_reconstruction_error(const std::vector<float>& original, const std::vector<float>& reconstructed) const override { return 0.0f; }
    // LATENT_DIM temel sınıfta static constexpr olduğu için burada override edilmez.
};
static DummyCryptofigAutoencoder dummy_autoencoder;

// 3. DummyCryptofigProcessor (Temel sınıftan türetme ve kurucu çağırma)
class DummyCryptofigProcessor : public CryptofigProcessor {
public:
    DummyCryptofigProcessor(IntentAnalyzer& analyzer_ref, CryptofigAutoencoder& autoencoder_ref)
        : CryptofigProcessor(analyzer_ref, autoencoder_ref) {}
    virtual void process_expert_cryptofig(const std::vector<float>& expert_cryptofig, IntentLearner& learner) override {}
    virtual std::vector<float> generate_cryptofig_from_signals(const DynamicSequence& sequence) override { return {0.0f, 0.0f, 0.0f}; }
    virtual CryptofigAutoencoder& get_autoencoder() override { return dummy_autoencoder; } // Const olmayan versiyon
    virtual const CryptofigAutoencoder& get_autoencoder() const override { return dummy_autoencoder; } // Const versiyon
};
static DummyCryptofigProcessor dummy_cryptofig_processor(dummy_analyzer, dummy_autoencoder);

// 4. DummyIntentLearner (Temel sınıftan türetme ve kurucu çağırma)
class DummyIntentLearner : public IntentLearner {
public:
    DummyIntentLearner(IntentAnalyzer& analyzer_ref) : IntentLearner(analyzer_ref) {}
    virtual void process_explicit_feedback(UserIntent predicted_intent, AIAction action, bool approved, const DynamicSequence& sequence) override {}
    virtual AbstractState infer_abstract_state(const std::deque<AtomicSignal>& recent_signals) override { return AbstractState::None; }
};
static DummyIntentLearner dummy_learner(dummy_analyzer);

// 5. DummyPredictionEngine (Temel sınıftan türetme ve kurucu çağırma)
// PredictionEngine'ın kurucusu IntentAnalyzer ve SequenceManager referansları bekler.
static SequenceManager dummy_sequence_manager; // SequenceManager'ın varsayılan kurucusu var
class DummyPredictionEngine : public PredictionEngine {
public:
    DummyPredictionEngine() : PredictionEngine(dummy_analyzer, dummy_sequence_manager) {}
    virtual UserIntent predict_next_intent(UserIntent current_intent, const DynamicSequence& sequence) const override { return current_intent; }
};
static DummyPredictionEngine dummy_predictor;

// 6. DummyAIInsightsEngine (Temel sınıftan türetme ve kurucu çağırma)
class DummyAIInsightsEngine : public AIInsightsEngine {
public:
    DummyAIInsightsEngine(IntentAnalyzer& analyzer_ref, IntentLearner& learner_ref,
                          PredictionEngine& predictor_ref, CryptofigAutoencoder& autoencoder_ref,
                          CryptofigProcessor& cryptofig_processor_ref)
        : AIInsightsEngine(analyzer_ref, learner_ref, predictor_ref, autoencoder_ref, cryptofig_processor_ref) {}
    virtual std::vector<AIInsight> generate_insights(const DynamicSequence& sequence) override { return {}; }
    virtual IntentAnalyzer& get_analyzer() const override { return dummy_analyzer; }
};
static DummyAIInsightsEngine dummy_insights_engine(dummy_analyzer, dummy_learner, dummy_predictor, dummy_autoencoder, dummy_cryptofig_processor);

// 7. DummyGoalManager (Temel sınıftan türetme ve kurucu çağırma)
class DummyGoalManager : public GoalManager {
public:
    DummyGoalManager(AIInsightsEngine& insights_engine_ref) : GoalManager(insights_engine_ref) {}
    virtual AIGoal get_current_goal() const override { return AIGoal::None; }
};
static DummyGoalManager dummy_goal_manager(dummy_insights_engine);


int main() {
    LOG_INIT(L"test_response_engine.log"); // L ekledik ve makro artık tanımlı olmalı
    LOG(LogLevel::INFO, std::wcout, L"ResponseEngine Test Başladı.");

    // Statik olarak tanımlanmış dummy bağımlılıkları ResponseEngine'a geçirin
    ResponseEngine response_engine(dummy_analyzer, dummy_goal_manager, dummy_insights_engine);

    std::wcout << L"--- ResponseEngine Testleri ---" << std::endl << std::endl;

    // --- Test Case 1: Greeting Intent (Rastgele Yanıt Seçimi) ---
    DynamicSequence seq_greeting;
    seq_greeting.latent_cryptofig_vector = {0.1f, 0.1f, 0.1f}; // Düşük aktiflik, karmaşıklık, etkileşim

    std::wcout << L"Test Durumu 1 (Selamlama Niyeti - Rastgele Yanıt Seçimi):" << std::endl;
    std::wcout << L"Beklenen: Farklı selamlama yanıtları (manuel gözlem)" << std::endl;
    std::set<std::wstring> greeting_responses;
    for (int i = 0; i < 5; ++i) {
        std::wstring response = response_engine.generate_response(UserIntent::None, AbstractState::None, AIGoal::None, seq_greeting);
        std::wcout << L"  Yanıt " << i + 1 << L": " << response << std::endl;
        greeting_responses.insert(response);
    }
    if (greeting_responses.size() > 1) {
        std::wcout << L"  -> BAŞARILI: Birden fazla farklı yanıt gözlemlendi." << std::endl;
    } else {
        std::wcout << L"  -> BAŞARISIZ: Yalnızca tek bir yanıt gözlemlendi (rastgele seçim çalışmıyor olabilir)." << std::endl;
    }
    std::wcout << std::endl;

    // --- Test Case 2: Low Productivity State ---
    DynamicSequence seq_low_perf;
    seq_low_perf.latent_cryptofig_vector = {0.5f, 0.8f, 0.4f}; // Yüksek karmaşıklık
    seq_low_perf.current_battery_percentage = 15;
    seq_low_perf.current_battery_charging = false;

    std::wcout << L"Test Durumu 2 (Düşük Üretkenlik Durumu):" << std::endl;
    std::wcout << L"Beklenen: 'Sistem performansınız düşük görünüyor...' veya 'Bataryanız azalıyor...' gibi bir yanıt." << std::endl;
    std::wstring response_low_perf = response_engine.generate_response(UserIntent::None, AbstractState::LowProductivity, AIGoal::None, seq_low_perf);
    std::wcout << L"  Yanıt: " << response_low_perf << std::endl;
    if (response_low_perf.find(L"Sistem performansınız düşük görünüyor") != std::wstring::npos ||
        response_low_perf.find(L"Bataryanız azalıyor") != std::wstring::npos) {
        std::wcout << L"  -> BAŞARILI: Beklenen yanıt türü alındı." << std::endl;
    } else {
        std::wcout << L"  -> BAŞARISIZ: Beklenen yanıt türü alınamadı." << std::endl;
    }
    std::wcout << std::endl;

    // --- Test Case 3: High Latent Complexity (Yeni Entegrasyon) ---
    DynamicSequence seq_high_complexity;
    seq_high_complexity.latent_cryptofig_vector = {0.6f, 0.9f, 0.5f}; // latent_complexity > 0.7f

    std::wcout << L"Test Durumu 3 (Yüksek Latent Karmaşıklık - Yeni Entegrasyon):" << std::endl;
    std::wcout << L"Beklenen: 'Bu durum oldukça karmaşık görünüyor...' ifadesini içeren bir yanıt." << std::endl;
    std::wstring response_high_complexity = response_engine.generate_response(UserIntent::Unknown, AbstractState::None, AIGoal::None, seq_high_complexity);
    std::wcout << L"  Yanıt: " << response_high_complexity << std::endl;
    if (response_high_complexity.find(L"Bu durum oldukça karmaşık görünüyor, daha fazla detaya ihtiyacım olabilir.") != std::wstring::npos) {
        std::wcout << L"  -> BAŞARILI: Yüksek latent karmaşıklık mesajı eklendi." << std::endl;
    } else {
        std::wcout << L"  -> BAŞARISIZ: Yüksek latent karmaşıklık mesajı eklenmedi." << std::endl;
    }
    std::wcout << std::endl;

    // --- Test Case 4: Low Latent Complexity (Yeni Entegrasyon - Mesaj Eklenmemeli) ---
    DynamicSequence seq_low_complexity;
    seq_low_complexity.latent_cryptofig_vector = {0.6f, 0.5f, 0.5f}; // latent_complexity < 0.7f

    std::wcout << L"Test Durumu 4 (Düşük Latent Karmaşıklık - Mesaj Eklenmemeli):" << std::endl;
    std::wcout << L"Beklenen: 'Bu durum oldukça karmaşık görünüyor...' ifadesini İÇERMEYEN bir yanıt." << std::endl;
    std::wstring response_low_complexity = response_engine.generate_response(UserIntent::Unknown, AbstractState::None, AIGoal::None, seq_low_complexity);
    std::wcout << L"  Yanıt: " << response_low_complexity << std::endl;
    if (response_low_complexity.find(L"Bu durum oldukça karmaşık görünüyor, daha fazla detaya ihtiyacım olabilir.") == std::wstring::npos) {
        std::wcout << L"  -> BAŞARILI: Düşük latent karmaşıklık mesajı eklenmedi." << std::endl;
    } else {
        std::wcout << L"  -> BAŞARISIZ: Düşük latent karmaşıklık mesajı yanlışlıkla eklendi." << std::endl;
    }
    std::wcout << std::endl;

    // --- Test Case 5: Low Latent Activity and Engagement (General Observation) ---
    DynamicSequence seq_low_activity_engagement;
    seq_low_activity_engagement.latent_cryptofig_vector = {0.2f, 0.4f, 0.1f}; // Düşük aktiflik ve etkileşim
    std::wcout << L"Test Durumu 5 (Düşük Latent Aktiflik/Etkileşim):" << std::endl;
    std::wcout << L"Beklenen: 'Latent analizim düşük aktiflik ve etkileşim görüyor...' gibi bir yanıt." << std::endl;
    std::wstring response_low_activity_engagement = response_engine.generate_response(UserIntent::Unknown, AbstractState::None, AIGoal::None, seq_low_activity_engagement);
    std::wcout << L"  Yanıt: " << response_low_activity_engagement << std::endl;
    if (response_low_activity_engagement.find(L"Latent analizim düşük aktiflik ve etkileşim görüyor") != std::wstring::npos) {
        std::wcout << L"  -> BAŞARILI: Beklenen yanıt türü alındı." << std::endl;
    } else {
        std::wcout << L"  -> BAŞARISIZ: Beklenen yanıt türü alınamadı." << std::endl;
    }
    std::wcout << std::endl;

    return 0;
}