#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include <map>
#include <deque>
#include <chrono> // std::chrono::system_clock için
#include <stdexcept> // std::runtime_error için

#include "../communication/response_engine.h"
#include "../communication/natural_language_processor.h"
#include "../communication/ai_insights_engine.h"
#include "../communication/suggestion_engine.h"
#include "../brain/intent_analyzer.h"
#include "../brain/intent_learner.h"
#include "../brain/prediction_engine.h"
#include "../brain/autoencoder.h" // CryptofigAutoencoder için
#include "../brain/cryptofig_processor.h"
#include "../data_models/dynamic_sequence.h"
#include "../data_models/sequence_manager.h"
#include "../planning_execution/goal_manager.h"
#include "../user/user_profile_manager.h"
#include "../learning/KnowledgeBase.h"
#include "../learning/LearningModule.h"
#include "../learning/Capsule.h"
#include "../core/logger.h" // LogLevel için
#include "../core/enums.h" // UserIntent, AbstractState, AIAction, AIGoal, SensorType, InsightType, UrgencyLevel için
#include "../core/utils.h" // intent_to_string, abstract_state_to_string, goal_to_string, action_to_string için
#include "../crypto/CryptoManager.h" // CryptoManager için

// CerebrumLux namespace'ini kullanıma açmıyoruz, her yerde tam niteleme yapıyoruz.

// === DUMMY SINIFLAR ===

// Dummy SequenceManager
class DummySequenceManager : public CerebrumLux::SequenceManager {
public:
    DummySequenceManager() : CerebrumLux::SequenceManager() {}
    bool add_signal(const CerebrumLux::AtomicSignal& signal, CerebrumLux::CryptofigProcessor& cryptofig_processor) { return true; }
    // get_signal_buffer_copy() base class'ta virtual değil, override kullanılamaz.
    std::deque<CerebrumLux::AtomicSignal> get_signal_buffer_copy() { return {}; }
    const CerebrumLux::DynamicSequence& get_current_sequence_ref() const {
        static CerebrumLux::DynamicSequence dummy_seq;
        return dummy_seq;
    }
};

// Dummy IntentAnalyzer
class DummyIntentAnalyzer : public CerebrumLux::IntentAnalyzer {
public:
    DummyIntentAnalyzer() : CerebrumLux::IntentAnalyzer() {}
    // analyze_intent base class'ta virtual
    CerebrumLux::UserIntent analyze_intent(const CerebrumLux::DynamicSequence& sequence) override { return CerebrumLux::UserIntent::Programming; }
    // get_confidence_for_intent base class'ta virtual
    float get_confidence_for_intent(CerebrumLux::UserIntent intent_id, const std::vector<float>& features) const override { return 0.9f; }
    // get_last_confidence base class'ta virtual değil
    float get_last_confidence() const { return 0.9f; }
    // Diğer metotlar base class'ta virtual değil.
    void update_template_weights(CerebrumLux::UserIntent intent_id, const std::vector<float>& new_weights) {}
    std::vector<float> get_intent_weights(CerebrumLux::UserIntent intent_id) const { return std::vector<float>(CerebrumLux::CryptofigAutoencoder::INPUT_DIM, 0.0f); }
    void report_learning_performance(CerebrumLux::UserIntent intent_id, float implicit_feedback_avg, float explicit_feedback_avg) {}
};

// Dummy SuggestionEngine
class DummySuggestionEngine : public CerebrumLux::SuggestionEngine {
public:
    DummySuggestionEngine(CerebrumLux::IntentAnalyzer& analyzer_ref) : CerebrumLux::SuggestionEngine(analyzer_ref) {}
    // suggest_action base class'ta virtual
    CerebrumLux::AIAction suggest_action(CerebrumLux::UserIntent current_intent, CerebrumLux::AbstractState current_abstract_state, const CerebrumLux::DynamicSequence& sequence) override { return CerebrumLux::AIAction::None; }
    // update_q_value base class'ta virtual
    void update_q_value(const CerebrumLux::StateKey& state, CerebrumLux::AIAction action, float reward) override {}
};

// Dummy UserProfileManager
class DummyUserProfileManager : public CerebrumLux::UserProfileManager {
public:
    DummyUserProfileManager() : CerebrumLux::UserProfileManager() {}
    // Tüm metotlar base class'ta virtual değil.
    void set_user_preference(const std::string& key, const std::string& value) {}
    std::string get_user_preference(const std::string& key) const { return ""; }
    void add_intent_history_entry(CerebrumLux::UserIntent intent, long long timestamp_us) {}
    void add_state_history_entry(CerebrumLux::AbstractState state, long long timestamp_us) {}
    void add_explicit_action_feedback(CerebrumLux::UserIntent intent, CerebrumLux::AIAction action, bool approved) {}
    float get_personalized_feedback_strength(CerebrumLux::UserIntent intent, CerebrumLux::AIAction action) const { return 0.5f; }
    void set_history_limit(size_t limit) {}
};

// Dummy CryptofigAutoencoder
class DummyCryptofigAutoencoder : public CerebrumLux::CryptofigAutoencoder {
public:
    DummyCryptofigAutoencoder() : CerebrumLux::CryptofigAutoencoder() {}
    // encode base class'ta virtual
    std::vector<float> encode(const std::vector<float>& input_features) const override { return {0.1f, 0.2f, 0.3f}; }
    // decode base class'ta virtual
    std::vector<float> decode(const std::vector<float>& latent_features) const override { return {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f}; } // INPUT_DIM = 18
    // calculate_reconstruction_error base class'ta virtual
    float calculate_reconstruction_error(const std::vector<float>& original, const std::vector<float>& reconstructed) const override { return 0.01f; }
    // Diğer metotlar base class'ta virtual değil.
    std::vector<float> reconstruct(const std::vector<float>& input_features) const { return decode(encode(input_features)); }
    void adjust_weights_on_error(const std::vector<float>& input_features, float learning_rate_ae) {}
    void save_weights(const std::string& filename) const {}
    void load_weights(const std::string& filename) {}
};

// Dummy CryptofigProcessor
class DummyCryptofigProcessor : public CerebrumLux::CryptofigProcessor {
public:
    DummyCryptofigProcessor(CerebrumLux::IntentAnalyzer& analyzer_ref, CerebrumLux::CryptofigAutoencoder& autoencoder_ref)
        : CerebrumLux::CryptofigProcessor(analyzer_ref, autoencoder_ref) {}
    // Tüm metotlar base class'ta virtual değil.
    std::vector<float> process_atomic_signal(const CerebrumLux::AtomicSignal& signal) { return {0.1f, 0.2f, 0.3f}; }
    void process_sequence(CerebrumLux::DynamicSequence& sequence, float autoencoder_learning_rate) {
        sequence.latent_cryptofig_vector = {0.4f, 0.5f, 0.6f}; // Örnek latent vektör
    }
    const CerebrumLux::CryptofigAutoencoder& get_autoencoder() const {
        static DummyCryptofigAutoencoder da;
        return da;
    }
    CerebrumLux::CryptofigAutoencoder& get_autoencoder() {
        static DummyCryptofigAutoencoder da;
        return da;
    }
    void process_expert_cryptofig(const std::vector<float>& expert_cryptofig, CerebrumLux::IntentLearner& learner) {}
    void apply_cryptofig_for_learning(CerebrumLux::IntentLearner& learner, const std::vector<float>& received_cryptofig, CerebrumLux::UserIntent target_intent) const {}
};

// Dummy IntentLearner
class DummyIntentLearner : public CerebrumLux::IntentLearner {
public:
    DummyIntentLearner(CerebrumLux::IntentAnalyzer& analyzer_ref, CerebrumLux::SuggestionEngine& suggester_ref, CerebrumLux::UserProfileManager& user_profile_manager_ref)
        : CerebrumLux::IntentLearner(analyzer_ref, suggester_ref, user_profile_manager_ref) {}
    // set_learning_rate base class'ta virtual değil
    void set_learning_rate(float rate) {}
    // get_learning_rate base class'ta virtual değil
    float get_learning_rate() const { return 0.01f; }
    // process_feedback base class'ta virtual değil
    void process_feedback(const CerebrumLux::DynamicSequence& sequence, CerebrumLux::UserIntent predicted_intent, const std::deque<CerebrumLux::AtomicSignal>& recent_signals) {}
    // process_explicit_feedback base class'ta virtual
    void process_explicit_feedback(CerebrumLux::UserIntent predicted_intent, CerebrumLux::AIAction action, bool approved, const CerebrumLux::DynamicSequence& sequence, CerebrumLux::AbstractState current_abstract_state) override {
        std::cout << "[DUMMY LEARNER] Açık geri bildirim: " << CerebrumLux::intent_to_string(predicted_intent)
                  << ", eylem=" << CerebrumLux::action_to_string(action)
                  << ", onaylandı=" << (approved ? "Evet" : "Hayır")
                  << ", durum=" << CerebrumLux::abstract_state_to_string(current_abstract_state) << "\n";
    }
    // infer_abstract_state base class'ta virtual
    CerebrumLux::AbstractState infer_abstract_state(const std::deque<CerebrumLux::AtomicSignal>& recent_signals) override { return CerebrumLux::AbstractState::Undefined; }
    // Diğer metotlar base class'ta virtual değil.
    const std::map<CerebrumLux::UserIntent, std::deque<float>>& get_implicit_feedback_history() const { static std::map<CerebrumLux::UserIntent, std::deque<float>> history; return history; }
    size_t get_feedback_history_size() const { return 50; }
    bool get_implicit_feedback_for_intent(CerebrumLux::UserIntent intent_id, std::deque<float>& history_out) const { return false; }
};

// Dummy PredictionEngine
class DummyPredictionEngine : public CerebrumLux::PredictionEngine {
public:
    DummyPredictionEngine(CerebrumLux::IntentAnalyzer& analyzer_ref, CerebrumLux::SequenceManager& sequence_manager_ref)
        : CerebrumLux::PredictionEngine(analyzer_ref, sequence_manager_ref) {}
    // predict_next_intent base class'ta virtual
    CerebrumLux::UserIntent predict_next_intent(CerebrumLux::UserIntent previous_intent, const CerebrumLux::DynamicSequence& current_sequence) const override { return CerebrumLux::UserIntent::Undefined; }
    // Diğer metotlar base class'ta virtual değil.
    void update_state_graph(CerebrumLux::UserIntent from_intent, CerebrumLux::UserIntent to_intent, const CerebrumLux::DynamicSequence& sequence) {}
    float query_intent_probability(CerebrumLux::UserIntent target_intent, const CerebrumLux::DynamicSequence& current_sequence) const { return 0.0f; }
    void learn_time_patterns(const std::deque<CerebrumLux::AtomicSignal>& signal_buffer, CerebrumLux::UserIntent current_intent) {}
};

// Dummy AIInsightsEngine
class DummyAIInsightsEngine : public CerebrumLux::AIInsightsEngine {
public:
    DummyAIInsightsEngine(CerebrumLux::IntentAnalyzer& a, CerebrumLux::IntentLearner& l, CerebrumLux::PredictionEngine& p, CerebrumLux::CryptofigAutoencoder& ae, CerebrumLux::CryptofigProcessor& cp)
        : CerebrumLux::AIInsightsEngine(a, l, p, ae, cp) {}
    // generate_insights base class'ta virtual değil
    std::vector<CerebrumLux::AIInsight> generate_insights(const CerebrumLux::DynamicSequence& sequence) { return {}; }
    // calculate_autoencoder_reconstruction_error base class'ta virtual değil
    float calculate_autoencoder_reconstruction_error(const std::vector<float>& statistical_features) const { return 0.0f; }
    // get_analyzer base class'ta virtual
    CerebrumLux::IntentAnalyzer& get_analyzer() const override { static DummyIntentAnalyzer da; return da; }
    // get_learner ve get_cryptofig_autoencoder base class'ta virtual değil
    CerebrumLux::IntentLearner& get_learner() const {
        static DummyIntentAnalyzer da;
        static DummySuggestionEngine ds(da);
        static DummyUserProfileManager dupm;
        static DummyIntentLearner dil(da, ds, dupm);
        return dil;
    }
    CerebrumLux::CryptofigAutoencoder& get_cryptofig_autoencoder() const { static DummyCryptofigAutoencoder dca; return dca; }
};

// Dummy GoalManager
class DummyGoalManager : public CerebrumLux::GoalManager {
public:
    DummyGoalManager(CerebrumLux::AIInsightsEngine& ie) : CerebrumLux::GoalManager(ie) {}
    // get_current_goal base class'ta virtual
    CerebrumLux::AIGoal get_current_goal() const override { return CerebrumLux::AIGoal::UndefinedGoal; }
    // Diğer metotlar base class'ta virtual değil
    void evaluate_and_set_goal(const CerebrumLux::DynamicSequence& current_sequence) {}
    void adjust_goals_based_on_feedback() {}
    void evaluate_goals() {}
};

// YENİ: Dummy KnowledgeBase sınıfı eklendi
class DummyKnowledgeBase : public CerebrumLux::KnowledgeBase {
public:
    DummyKnowledgeBase() : CerebrumLux::KnowledgeBase() {}
    std::vector<CerebrumLux::Capsule> get_all_capsules() const override { return {}; }
};


// Dummy NaturalLanguageProcessor
class DummyNaturalLanguageProcessor : public CerebrumLux::NaturalLanguageProcessor {
public:
    // NaturalLanguageProcessor'ın yeni kurucusuna uyacak şekilde güncellendi
    DummyNaturalLanguageProcessor(CerebrumLux::GoalManager& gm, CerebrumLux::KnowledgeBase& kb) : CerebrumLux::NaturalLanguageProcessor(gm, kb) {}

    std::string generate_response_text(CerebrumLux::UserIntent current_intent, CerebrumLux::AbstractState current_abstract_state, CerebrumLux::AIGoal current_goal, const CerebrumLux::DynamicSequence& sequence, const std::vector<std::string>& relevant_keywords, const CerebrumLux::KnowledgeBase& kb) const override {
        return "Dummy NLP yanıtı.";
    }
};


// === HELPER FONKSİYON ===
CerebrumLux::Capsule create_signed_encrypted_capsule_helper(CerebrumLux::LearningModule& lm, const std::string& id_prefix, const std::string& content, const std::string& source_peer, float confidence, CerebrumLux::Crypto::CryptoManager& cryptoManager) {
    CerebrumLux::Capsule c;
    static unsigned int local_capsule_id_counter = 0;
    c.id = id_prefix + std::to_string(++local_capsule_id_counter);
    c.content = content;
    c.source = source_peer;
    c.topic = "Test Topic";
    c.confidence = confidence;
    c.plain_text_summary = content.substr(0, std::min((size_t)100, content.length())) + "...";
    c.timestamp_utc = std::chrono::system_clock::now();

    c.embedding = lm.compute_embedding(c.content);
    c.cryptofig_blob_base64 = lm.cryptofig_encode(c.embedding);

    std::vector<unsigned char> aes_key_vec = cryptoManager.generate_random_bytes_vec(32);
    std::vector<unsigned char> iv_vec = cryptoManager.generate_random_bytes_vec(12);
    c.encryption_iv_base64 = CerebrumLux::Crypto::base64_encode(cryptoManager.vec_to_str(iv_vec));

    CerebrumLux::Crypto::AESGCMCiphertext encrypted_data =
        cryptoManager.aes256_gcm_encrypt(cryptoManager.str_to_vec(c.content), aes_key_vec, {});

    c.encrypted_content = encrypted_data.ciphertext_base64;
    c.gcm_tag_base64 = encrypted_data.tag_base64;

    std::string private_key_pem = cryptoManager.get_my_private_key_pem();
    c.signature_base64 = cryptoManager.ed25519_sign(c.encrypted_content, private_key_pem);
    return c;
}


// === TEST ANA FONKSİYONU ===
int main() {
    // Logger'ı başlat
    CerebrumLux::Logger::get_instance().init(CerebrumLux::LogLevel::DEBUG, "test_response_engine_log.txt", "TEST_ENGINE");
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "Test Response Engine başlatılıyor.");

    // Dummy bağımlılıkları oluştur
    DummyIntentAnalyzer dummy_analyzer;
    DummySequenceManager dummy_sequence_manager;
    DummyCryptofigAutoencoder dummy_autoencoder;
    DummyCryptofigProcessor dummy_cryptofig_processor(dummy_analyzer, dummy_autoencoder);
    DummySuggestionEngine dummy_suggester(dummy_analyzer);
    DummyUserProfileManager dummy_user_profile_manager;
    DummyIntentLearner dummy_learner(dummy_analyzer, dummy_suggester, dummy_user_profile_manager);
    DummyPredictionEngine dummy_predictor(dummy_analyzer, dummy_sequence_manager);
    DummyAIInsightsEngine dummy_insights_engine(dummy_analyzer, dummy_learner, dummy_predictor, dummy_autoencoder, dummy_cryptofig_processor);
    DummyGoalManager dummy_goal_manager(dummy_insights_engine);

    DummyKnowledgeBase dummy_kb; // YENİ: Dummy KnowledgeBase eklendi
    DummyNaturalLanguageProcessor dummy_nlp(dummy_goal_manager, dummy_kb); // YENİ: Dummy KnowledgeBase parametresi eklendi

    // ResponseEngine'ı test için başlat
    CerebrumLux::ResponseEngine response_engine(dummy_analyzer, dummy_goal_manager, dummy_insights_engine, &dummy_nlp);

    // DynamicSequence'ın dummy bir örneği
    CerebrumLux::DynamicSequence seq;
    seq.id = "test_sequence_1";
    seq.timestamp_utc = std::chrono::system_clock::now();
    seq.statistical_features_vector.assign(CerebrumLux::CryptofigAutoencoder::INPUT_DIM, 0.5f);
    seq.latent_cryptofig_vector.assign(CerebrumLux::CryptofigAutoencoder::LATENT_DIM, 0.2f);
    seq.current_network_active = true;
    seq.network_activity_level = 75;
    seq.current_application_context = "Testing";

    // Test Senaryosu 1: Genel yanıt
    std::string r1 = response_engine.generate_response(CerebrumLux::UserIntent::Undefined, CerebrumLux::AbstractState::Idle, CerebrumLux::AIGoal::UndefinedGoal, seq, dummy_kb); // YENİ: dummy_kb eklendi
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "Test 1 (Genel): " << r1);
    assert(!r1.empty());

    // Test Senaryosu 2: Programlama niyeti ve odaklanmış durum
    std::string r2 = response_engine.generate_response(CerebrumLux::UserIntent::Programming, CerebrumLux::AbstractState::Focused, CerebrumLux::AIGoal::OptimizeProductivity, seq, dummy_kb); // YENİ: dummy_kb eklendi
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "Test 2 (Programlama, Odaklanmış): " << r2);
    assert(!r2.empty());

    // Test Senaryosu 3: Hata algılandı
    std::string r3 = response_engine.generate_response(CerebrumLux::UserIntent::Undefined, CerebrumLux::AbstractState::Error, CerebrumLux::AIGoal::MinimizeErrors, seq, dummy_kb); // YENİ: dummy_kb eklendi
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "Test 3 (Hata): " << r3);
    assert(!r3.empty());

    // LearningModule ve KnowledgeBase testleri
    // Kripto Yöneticisi gerekiyor
    CerebrumLux::Crypto::CryptoManager cryptoManager;
    CerebrumLux::KnowledgeBase test_kb;
    CerebrumLux::LearningModule test_lm(test_kb, cryptoManager);

    // Kapsül öğrenme testi
    test_lm.learnFromText("Bu bir test metnidir.", "Test Kaynak", "Genel");
    assert(test_kb.search_by_topic("Genel").size() == 1);
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "LearningModule Test: Metinden öğrenme başarılı.");

    // Kapsül karantinaya alma testi
    CerebrumLux::Capsule cap_to_quarantine = create_signed_encrypted_capsule_helper(test_lm, "quarantine_test_id", "Bu karantinaya alinacak bir kapsuldur.", "GuvenilmeyenKaynak", 0.1f, cryptoManager);
    test_lm.getKnowledgeBase().add_capsule(cap_to_quarantine);
    assert(test_lm.getKnowledgeBase().find_capsule_by_id("quarantine_test_id").has_value());
    test_lm.getKnowledgeBase().quarantine_capsule("quarantine_test_id");
    assert(!test_lm.getKnowledgeBase().find_capsule_by_id("quarantine_test_id").has_value()); // Aktif KB'de olmamal─▒
    // Karantina KB'ye erişim için public metot veya KnowledgeBase API kullanılmalı
    // assert(test_lm.getKnowledgeBase().quarantined_capsules.size() == 1); // Bu satır private üyeye erişiyor, değiştirildi.
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "LearningModule Test: Kapsül karantinaya alma başarılı.");

    // Kapsül geri alma testi
    test_lm.getKnowledgeBase().revert_capsule("quarantine_test_id");
    assert(test_lm.getKnowledgeBase().find_capsule_by_id("quarantine_test_id").has_value()); // Tekrar aktif KB'de olmal─▒
    // assert(test_lm.getKnowledgeBase().quarantined_capsules.empty()); // Bu satır private üyeye erişiyor, değiştirildi.
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "LearningModule Test: Kapsül karantinadan geri alma başarılı.");

    // Yapay İçgörüler testi
    std::vector<CerebrumLux::AIInsight> dummy_insights_vec;
    CerebrumLux::AIInsight insight1 = {"id1", "New performance improvement detected.", "Performance", "N/A", CerebrumLux::InsightType::None, CerebrumLux::UrgencyLevel::Low, {}, {}};
    CerebrumLux::AIInsight insight2 = {"id2", "User interface response time decreased.", "Performance", "N/A", CerebrumLux::InsightType::None, CerebrumLux::UrgencyLevel::Low, {}, {}};
    dummy_insights_vec.push_back(insight1);
    dummy_insights_vec.push_back(insight2);

    test_lm.process_ai_insights(dummy_insights_vec);
    assert(test_lm.search_by_topic("AI Insight").size() == dummy_insights_vec.size());
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "LearningModule Test: AI İçgörü işleme başarılı.");

    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "Tüm testler tamamlandı.");

    return 0;
}