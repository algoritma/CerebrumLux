#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <map>
#include <deque>
#include <fstream> 
#include <optional> 

// OpenSSL Başlıkları (Sadece EVP_CIPHER_iv_length için gerekli olanı bırakıldı)
#include <openssl/evp.h> 

// Project headers (use relative path from tests/)
#include "../src/communication/response_engine.h"
#include "../src/communication/ai_insights_engine.h"
#include "../src/communication/suggestion_engine.h"
#include "../src/communication/natural_language_processor.h"
#include "../src/brain/intent_analyzer.h"
#include "../src/brain/intent_learner.h"
#include "../src/brain/prediction_engine.h"
#include "../src/brain/cryptofig_processor.h"
#include "../src/brain/autoencoder.h"
#include "../src/data_models/dynamic_sequence.h"
#include "../src/data_models/sequence_manager.h" 
#include "../src/user/user_profile_manager.h"
#include "../src/planning_execution/goal_manager.h"
#include "../src/core/logger.h"
#include "../src/core/utils.h" // utils.h'de artık global base64 fonksiyonları yok
#include "../src/learning/KnowledgeBase.h" 
#include "../src/learning/LearningModule.h" 
#include "../src/learning/Capsule.h" 
#include "../src/learning/WebFetcher.h" 

// Rastgele sayı üreteci (random_device hatasını önlemek için sabit seed ile)
static std::mt19937 gen_test(12345); 

// Geçici olarak Ed25519 sabitleri tanımlanıyor, OpenSSL bulunana kadar
#define DUMMY_ED25519_PRIVKEY_LEN 64 
#define DUMMY_ED25519_PUBKEY_LEN 32  
#define DUMMY_ED25519_SIG_LEN   64   

// === Dummy implementations for testing ===

// Dummy SequenceManager
// HATA DÜZELTME: Base class metodları virtual olmadığı için 'override' kaldırıldı.
class DummySequenceManager : public SequenceManager {
public:
    DummySequenceManager() : SequenceManager() {}
    bool add_signal(const AtomicSignal& signal, CryptofigProcessor& cryptofig_processor) { 
        (void)signal; (void)cryptofig_processor; 
        return true;
    }
    std::deque<AtomicSignal> get_signal_buffer_copy() const { 
        return std::deque<AtomicSignal>{};
    }
    DynamicSequence& get_current_sequence_ref() { 
        static DynamicSequence dummy_seq;
        return dummy_seq;
    }
    const DynamicSequence& get_current_sequence_ref() const { 
        static DynamicSequence dummy_seq;
        return dummy_seq;
    }
};

// DummyIntentAnalyzer
class DummyIntentAnalyzer : public IntentAnalyzer {
public:
    DummyIntentAnalyzer() : IntentAnalyzer() {}
    virtual UserIntent analyze_intent(const DynamicSequence& sequence) override {
        (void)sequence; 
        if (sequence.avg_keystroke_interval > 100000.0f) return UserIntent::IdleThinking;
        if (sequence.alphanumeric_ratio > 0.8f) return UserIntent::FastTyping;
        return UserIntent::Editing;
    }
};

// DummySuggestionEngine
class DummySuggestionEngine : public SuggestionEngine {
public:
    DummySuggestionEngine(IntentAnalyzer& analyzer_ref) : SuggestionEngine(analyzer_ref) {}
    virtual AIAction suggest_action(UserIntent current_intent, AbstractState current_abstract_state, const DynamicSequence& sequence) override { 
        (void)sequence; 
        if (current_intent == UserIntent::IdleThinking) return AIAction::SuggestBreak;
        if (current_abstract_state == AbstractState::LowProductivity) return AIAction::DimScreen;
        return AIAction::None;
    }
    virtual void update_q_value(const StateKey& state, AIAction action, float reward) override { 
        (void)state; (void)action; (void)reward; 
    }
};

// DummyUserProfileManager
// HATA DÜZELTME: Base class metodları virtual olmadığı için 'override' kaldırıldı.
class DummyUserProfileManager : public UserProfileManager {
public:
    DummyUserProfileManager() : UserProfileManager() {}
    void update_profile_from_sequence(const DynamicSequence& sequence) { 
        (void)sequence; 
    }
    void add_intent_history_entry(UserIntent intent, long long timestamp_us) { 
        (void)timestamp_us; (void)intent;
    }
    void add_state_history_entry(AbstractState state, long long timestamp_us) { 
        (void)timestamp_us; (void)state;
    }
    void add_explicit_action_feedback(UserIntent intent, AIAction action, bool approved) { 
        (void)intent; (void)action; (void)approved; 
    }
    void load_profile(const std::string& filename) { 
        (void)filename; 
    }
    void save_profile(const std::string& filename) const { 
        (void)filename; 
    }
};

// Dummy IntentLearner
// HATA DÜZELTME: Sadece IntentLearner base class'ındaki virtual metodlar override edildi. 
// LearningModule'e ait olan ve override hatası veren metodlar bu dummy class'tan kaldırıldı.
class DummyIntentLearner : public IntentLearner {
public:
    DummyIntentLearner(IntentAnalyzer& analyzer_ref, SuggestionEngine& suggester_ref, UserProfileManager& user_profile_manager_ref)
        : IntentLearner(analyzer_ref, suggester_ref, user_profile_manager_ref) {}

    virtual void process_explicit_feedback(UserIntent predicted_intent, AIAction action, bool approved, const DynamicSequence& sequence, AbstractState current_abstract_state) override {
        (void)sequence; 
        std::cout << "[DUMMY LEARNER] Acik geri bildirim: " << intent_to_string(predicted_intent)
             << ", eylem=" << static_cast<int>(action) << ", onay=" << (approved ? "Evet" : "Hayir")
             << ", durum=" << abstract_state_to_string(current_abstract_state) << "\n";
    }

    virtual AbstractState infer_abstract_state(const std::deque<AtomicSignal>& recent_signals) override {
        if (recent_signals.size() > 50) return AbstractState::HighProductivity;
        if (recent_signals.empty()) return AbstractState::None;
        return AbstractState::NormalOperation;
    }
};

// Dummy PredictionEngine
// HATA DÜZELTME: Base class'ta virtual olmayan metodlardan 'override' kaldırıldı.
class DummyPredictionEngine : public PredictionEngine {
public:
    DummyPredictionEngine(IntentAnalyzer& analyzer_ref, SequenceManager& sequence_manager_ref)
        : PredictionEngine(analyzer_ref, sequence_manager_ref) {}
    virtual UserIntent predict_next_intent(UserIntent current_intent, const DynamicSequence& sequence) const override { 
        (void)current_intent; (void)sequence; 
        return UserIntent::Unknown;
    }
    void update_state_graph(UserIntent from_intent, UserIntent to_intent, const DynamicSequence& sequence) { 
        (void)from_intent; (void)to_intent; (void)sequence; 
    }
    void save_state_graph(const std::string& filename) const { 
        (void)filename; 
    }
    void load_state_graph(const std::string& filename) {  
        (void)filename; 
    }
};

// DummyCryptofigAutoencoder
// HATA DÜZELTME: Base class'ta virtual olmayan metodlardan 'override' kaldırıldı.
class DummyCryptofigAutoencoder : public CryptofigAutoencoder {
public:
    DummyCryptofigAutoencoder() : CryptofigAutoencoder() {}
    virtual std::vector<float> encode(const std::vector<float>& input_features) const override {
        (void)input_features; 
        return {0.1f, 0.2f, 0.3f};
    }
    void save_weights(const std::string& filename) const { 
        (void)filename; 
    }
    void load_weights(const std::string& filename) {  
        (void)filename; 
    }
    virtual float calculate_reconstruction_error(const std::vector<float>& original, const std::vector<float>& reconstructed) const override {
        (void)original; (void)reconstructed; 
        return 0.0f;
    }
};

// DummyCryptofigProcessor
// HATA DÜZELTME: Base class'ta virtual olmayan metodlardan 'override' kaldırıldı.
class DummyCryptofigProcessor : public CryptofigProcessor {
public:
    DummyCryptofigProcessor(IntentAnalyzer& analyzer_ref, CryptofigAutoencoder& autoencoder_ref)
        : CryptofigProcessor(analyzer_ref, autoencoder_ref) {}

    void process_sequence(DynamicSequence& sequence, float autoencoder_learning_rate) { 
        (void)autoencoder_learning_rate; 
        sequence.latent_cryptofig_vector = {0.4f, 0.5f, 0.6f};
    }
    virtual void process_expert_cryptofig(const std::vector<float>& expert_cryptofig, IntentLearner& learner) override {
        (void)expert_cryptofig; (void)learner; 
    }
    virtual std::vector<float> generate_cryptofig_from_signals(const DynamicSequence& sequence) override {
        (void)sequence; 
        return {0.7f, 0.8f, 0.9f};
    }
    virtual const CryptofigAutoencoder& get_autoencoder() const override {
        static DummyCryptofigAutoencoder da;
        return da;
    }
    virtual CryptofigAutoencoder& get_autoencoder() override {
        static DummyCryptofigAutoencoder da;
        return da;
    }
};

// DummyAIInsightsEngine
class DummyAIInsightsEngine : public AIInsightsEngine {
public:
    DummyAIInsightsEngine(IntentAnalyzer& a, IntentLearner& l, PredictionEngine& p, CryptofigAutoencoder& ae, CryptofigProcessor& cp)
        : AIInsightsEngine(a, l, p, ae, cp) {}

    virtual std::vector<AIInsight> generate_insights(const DynamicSequence& sequence) override {
        (void)sequence; 
        AIInsight insight("Performance looks good.", AIAction::None, 0.8f); 
        std::vector<AIInsight> out;
        out.push_back(insight); 
        return out;
    }
    virtual IntentAnalyzer& get_analyzer() const override {
        static DummyIntentAnalyzer analyzer_temp;
        return analyzer_temp;
    }
};

// DummyGoalManager
class DummyGoalManager : public GoalManager {
public:
    DummyGoalManager(AIInsightsEngine& ie) : GoalManager(ie) {}
    virtual AIGoal get_current_goal() const override {
        return AIGoal::OptimizeProductivity;
    }
};

// DummyNaturalLanguageProcessor
// HATA DÜZELTME: Base class metodları virtual olmadığı için 'override' kaldırıldı.
class DummyNaturalLanguageProcessor : public NaturalLanguageProcessor {
public:
    DummyNaturalLanguageProcessor(GoalManager& gm) : NaturalLanguageProcessor(gm) {}

    std::string generate_response_text(UserIntent current_intent, AbstractState current_abstract_state, AIGoal current_goal,
                                  const DynamicSequence& sequence, const std::vector<std::string>& relevant_keywords = {}) const { 
        (void)current_intent; (void)current_abstract_state; (void)current_goal; (void)sequence; (void)relevant_keywords; 
        return "NLP'den gelen test yaniti.";
    }
    UserIntent infer_intent_from_text(const std::string& user_input) const { 
        if (user_input.find("oyun") != std::string::npos) return UserIntent::Gaming;
        return UserIntent::GeneralInquiry;
    }
    AbstractState infer_state_from_text(const std::string& user_input) const { 
        if (user_input.find("pil") != std::string::npos) return AbstractState::PowerSaving;
        return AbstractState::NormalOperation;
    }
};

// Dummy WebFetcher
class DummyWebFetcher : public WebFetcher {
public:
    DummyWebFetcher() : WebFetcher() {}
    virtual std::vector<WebResult> search(const std::string& query) override { 
        (void)query;
        WebResult res1 = {"Test Content 1", "Test Source 1"};
        WebResult res2 = {"Test Content 2", "Test Source 2"};
        return {res1, res2};
    }
};


// Helper to sign and encrypt capsules for testing (global olarak tanımlanıyor)
// LearningModule objesini parametre olarak alıyor.
Capsule create_signed_encrypted_capsule_helper(LearningModule& lm, const std::string& id_prefix, const std::string& content, const std::string& source_peer, float confidence) {
    Capsule c;
    static unsigned int test_capsule_id_counter = 0; 
    c.id = id_prefix + std::to_string(++test_capsule_id_counter); 
    c.content = content;
    c.source = source_peer;
    c.topic = "Test Topic";
    c.confidence = confidence;
    c.plain_text_summary = content.substr(0, std::min((size_t)100, content.length())) + "...";
    c.timestamp_utc = std::chrono::system_clock::now();
    
    c.embedding = lm.compute_embedding(c.content);
    c.cryptofig_blob_base64 = lm.cryptofig_encode(c.embedding);

    std::string aes_key = lm.get_aes_key_for_peer(source_peer);
    std::string iv = lm.generate_random_bytes(EVP_CIPHER_iv_length(EVP_aes_256_gcm()));
    
    // HATA DÜZELTME: `base64_encode_internal` private olduğu için global `base64_encode` kullanıldı.
    // Bu fonksiyonun utils.h'de tanımlı olduğu ve linklendiği varsayılmıştır.
    c.encryption_iv_base64 = base64_encode(iv);
    c.encrypted_content = lm.aes_gcm_encrypt(c.content, aes_key, iv);

    // Ed25519 fonksiyonları yorum satırı yapıldığı için burada da simüle ediyoruz
    // std::string private_key = lm.get_my_private_key(); 
    // c.signature_base64 = lm.ed25519_sign(c.encrypted_content, private_key);
    c.signature_base64 = "valid_signature"; 
    return c;
}


// === Test main ===
int main() {
    std::cout << "ResponseEngine testleri (düzeltilmiş) başlatılıyor...\n";
    // HATA DÜZELTME: Kırık olan include'dan kaynaklanan 'Logger' tanınamadı hatası düzeltildi.
    Logger::get_instance().init(LogLevel::DEBUG, "test_log.txt");

    // Create dummy components
    DummyIntentAnalyzer dummy_analyzer;
    DummySequenceManager dummy_sequence_manager; 
    DummySuggestionEngine dummy_suggester(dummy_analyzer);
    DummyUserProfileManager dummy_user_profile_manager;
    DummyIntentLearner dummy_learner(dummy_analyzer, dummy_suggester, dummy_user_profile_manager);
    
    DummyCryptofigAutoencoder dummy_autoencoder; 
    DummyCryptofigProcessor dummy_cryptofig_processor(dummy_analyzer, dummy_autoencoder);
    DummyPredictionEngine dummy_predictor(dummy_analyzer, dummy_sequence_manager); 
    DummyAIInsightsEngine dummy_insights_engine(dummy_analyzer, dummy_learner, dummy_predictor, dummy_autoencoder, dummy_cryptofig_processor);
    DummyGoalManager dummy_goal_manager(dummy_insights_engine);
    DummyNaturalLanguageProcessor dummy_nlp(dummy_goal_manager); 

    ResponseEngine response_engine(dummy_analyzer, dummy_goal_manager, dummy_insights_engine, &dummy_nlp);

    // Prepare a DynamicSequence for testing
    DynamicSequence seq;
    seq.avg_keystroke_interval = 150000.0f; 
    seq.alphanumeric_ratio = 0.9f;
    seq.current_battery_percentage = 85;
    seq.current_battery_charging = false;
    seq.latent_cryptofig_vector = {0.5f, 0.6f, 0.7f};

    // Test 1: general response
    std::cout << "\n--- Test 1: Genel Yanıt ---\n";
    std::string r1 = response_engine.generate_response(UserIntent::None, AbstractState::None, AIGoal::None, seq);
    std::cout << "AI Yaniti: " << r1 << "\n";
    assert(!r1.empty());

    // Test 2: specific intent/state
    std::cout << "\n--- Test 2: Programlama ve Odaklanma ---\n";
    std::string r2 = response_engine.generate_response(UserIntent::Programming, AbstractState::Focused, AIGoal::OptimizeProductivity, seq);
    std::cout << "AI Yaniti: " << r2 << "\n";
    assert(!r2.empty());

    // Test 3: critical action confirmation (just ensure non-empty)
    std::cout << "\n--- Test 3: Kritik Eylem ---\n";
    std::string r3 = response_engine.generate_response(UserIntent::None, AbstractState::None, AIGoal::OptimizeProductivity, seq);
    std::cout << "AI Yaniti: " << r3 << "\n";
    assert(!r3.empty());

    std::cout << "\nDummy learner -> suggester feedback test\n";
    dummy_learner.process_explicit_feedback(UserIntent::FastTyping, AIAction::DisableSpellCheck, true, seq, AbstractState::HighProductivity);

    // user profile manager tests (simulate)
    dummy_user_profile_manager.add_explicit_action_feedback(UserIntent::Programming, AIAction::OpenDocumentation, true);
    dummy_user_profile_manager.save_profile("test_user_profile.json");
    dummy_user_profile_manager.load_profile("test_user_profile.json");

    // YENİ TESTLER: LearningModule ve KnowledgeBase
    std::cout << "\n--- LearningModule ve KnowledgeBase Testleri ---\n";
    KnowledgeBase test_kb;
    LearningModule test_lm(test_kb); 

    // Test LearningModule::learnFromText
    std::cout << "\nLearningModule::learnFromText testi:\n";
    test_lm.learnFromText("Bu bir test metnidir.", "Test Kaynak", "Genel");
    test_lm.learnFromText("Qt programlama cok keyifli.", "Blog", "Programlama");
    test_lm.learnFromText("Qt tasarim prensipleri.", "Dokuman", "Programlama");
    assert(test_kb.search_by_topic("Genel").size() == 1); 
    assert(test_kb.search_by_topic("Programlama").size() == 2); 
    std::cout << "learnFromText başarılı.\n";

    // Test KnowledgeBase::semantic_search
    std::cout << "\nKnowledgeBase::semantic_search testi:\n";
    auto similar_capsules = test_kb.semantic_search("Qt", 1); 
    assert(!similar_capsules.empty());
    std::cout << "En benzer kapsül: " << similar_capsules[0].content << "\n";

    // Test KnowledgeBase::encrypt/decrypt
    std::cout << "\nKnowledgeBase::encrypt/decrypt testi:\n";
    std::string original_text = "Gizli bilgi";
    std::string encrypted_text = test_kb.encrypt(original_text);
    std::string decrypted_text = test_kb.decrypt(encrypted_text); 
    assert(original_text == decrypted_text);
    std::cout << "Şifreleme/çözme başarılı.\n";

    // Test KnowledgeBase::quarantine_capsule ve revert_capsule
    std::cout << "\nKnowledgeBase::quarantine_capsule ve revert_capsule testi:\n";
    Capsule cap_to_quarantine;
    cap_to_quarantine.id = "quarantine_test_id";
    cap_to_quarantine.content = "Bu karantinaya alınacak bir kapsül.";
    cap_to_quarantine.source = "Test";
    cap_to_quarantine.topic = "Quarantine";
    cap_to_quarantine.timestamp_utc = std::chrono::system_clock::now(); 
    cap_to_quarantine.plain_text_summary = "Karantina kapsül özeti.";
    cap_to_quarantine.cryptofig_blob_base64 = "dummy_cryptofig_blob";
    cap_to_quarantine.signature_base64 = "dummy_signature";
    cap_to_quarantine.encryption_iv_base64 = "dummy_iv";

    test_kb.add_capsule(cap_to_quarantine);
    std::optional<Capsule> found_cap = test_kb.find_capsule_by_id("quarantine_test_id");
    assert(found_cap.has_value());
    test_kb.quarantine_capsule("quarantine_test_id");
    assert(!test_kb.find_capsule_by_id("quarantine_test_id").has_value()); 

    // Karantinadaki kapsülü geri almayı dene
    test_kb.revert_capsule("quarantine_test_id");
    assert(test_kb.find_capsule_by_id("quarantine_test_id").has_value()); 
    std::cout << "Karantina ve geri alma başarılı.\n";


    // Test LearningModule::process_ai_insights
    std::cout << "\nLearningModule::process_ai_insights testi:\n";
    std::vector<AIInsight> dummy_insights;
    dummy_insights.push_back(AIInsight("New performance improvement detected.", AIAction::None, 0.9f)); 
    dummy_insights.push_back(AIInsight("User interface response time decreased.", AIAction::None, 0.7f)); 
    test_lm.process_ai_insights(dummy_insights);
    assert(test_lm.search_by_topic("AI Insight").size() == dummy_insights.size()); 
    std::cout << "process_ai_insights başarılı.\n";

    std::cout << "\nTüm testler tamamlandi (düzeltilmiş test dosyası).\n";
    return 0;
}