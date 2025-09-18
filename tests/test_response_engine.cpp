#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <map>
#include <random> // std::mt19937 için
#include <deque>
#include <fstream> // KnowledgeBase dosya işlemleri için

// Project headers (use relative path from tests/)
#include "../src/communication/response_engine.h"
#include "../src/communication/ai_insights_engine.h"
#include "../src/communication/suggestion_engine.h"
#include "../src/communication/natural_language_processor.h"
#include "../src/brain/intent_analyzer.h"
#include "../src/brain/intent_learner.h"
#include "../src/brain/prediction_engine.h"
#include "../src/brain/cryptofig_processor.h" // DÜZELTİLDİ: #include eklendi
#include "../src/brain/autoencoder.h"
#include "../src/data_models/dynamic_sequence.h"
#include "../src/data_models/sequence_manager.h" 
#include "../src/user/user_profile_manager.h"
#include "../src/planning_execution/goal_manager.h"
#include "../src/core/logger.h" 
#include "../src/core/utils.h" // action_to_string, intent_to_string, abstract_state_to_string vb. için
#include "../src/learning/KnowledgeBase.h" 
#include "../src/learning/LearningModule.h" 
#include "../src/learning/Capsule.h" 
#include "../src/learning/WebFetcher.h" 

// Rastgele sayı üreteci (random_device hatasını önlemek için sabit seed ile)
static std::mt19937 gen_test(12345); 

// === Dummy implementations for testing ===

// Dummy SequenceManager deriving from real SequenceManager (so type is complete)
class DummySequenceManager : public SequenceManager {
public:
    DummySequenceManager() : SequenceManager() {}
    // Base class'ta virtual olmadığı için 'override' kaldırıldı
    bool add_signal(const AtomicSignal& signal, CryptofigProcessor& cryptofig_processor) /*override*/ { 
        (void)signal; (void)cryptofig_processor; 
        // std::cout << "[DUMMY SEQUENCE MANAGER] Sinyal eklendi (simüle edildi).\n";
        return true;
    }
    std::deque<AtomicSignal> get_signal_buffer_copy() const /*override*/ { 
        return std::deque<AtomicSignal>{};
    }
    // Base class'ta virtual olmadığı için 'override' kaldırıldı
    DynamicSequence& get_current_sequence_ref() /*override*/ { 
        static DynamicSequence dummy_seq; // Dummy nesne döndür
        return dummy_seq;
    }
    const DynamicSequence& get_current_sequence_ref() const /*override*/ { 
        static DynamicSequence dummy_seq; // Dummy nesne döndür
        return dummy_seq;
    }
};

// DummyIntentAnalyzer
class DummyIntentAnalyzer : public IntentAnalyzer {
public:
    DummyIntentAnalyzer() : IntentAnalyzer() {}
    // analyze_intent base class'ta virtual olduğu için 'override' anahtar kelimesi korundu
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
    // suggest_action base class'ta virtual olduğu için 'override' anahtar kelimesi korundu
    virtual AIAction suggest_action(UserIntent current_intent, AbstractState current_abstract_state, const DynamicSequence& sequence) override { 
        (void)sequence; 
        if (current_intent == UserIntent::IdleThinking) return AIAction::SuggestBreak;
        if (current_abstract_state == AbstractState::LowProductivity) return AIAction::DimScreen;
        return AIAction::None;
    }
    // update_q_value base class'ta virtual olduğu için 'override' anahtar kelimesi korundu
    virtual void update_q_value(const StateKey& state, AIAction action, float reward) override { 
        (void)state; (void)action; (void)reward; 
        // std::cout << "[DUMMY SUGGESTER] Q-degeri guncellendi (simule).\n";
    }
};

// DummyUserProfileManager
class DummyUserProfileManager : public UserProfileManager {
public:
    DummyUserProfileManager() : UserProfileManager() {}
    // Base class'ta virtual olmadığı için 'override' kaldırıldı
    void update_profile_from_sequence(const DynamicSequence& sequence) /*override*/ { 
        (void)sequence; 
        // std::cout << "[DUMMY USER PROFILE MANAGER] Profil guncellendi (simule).\n";
    }
    // Base class'ta virtual olmadığı için 'override' kaldırıldı
    void add_intent_history_entry(UserIntent intent, long long timestamp_us) /*override*/ { 
        (void)timestamp_us; 
        // std::cout << "[DUMMY USER PROFILE MANAGER] Niyet gecmisi eklendi: " << intent_to_string(intent) << "\n";
    }
    // Base class'ta virtual olmadığı için 'override' kaldırıldı
    void add_state_history_entry(AbstractState state, long long timestamp_us) /*override*/ { 
        (void)timestamp_us; 
        // std::cout << "[DUMMY USER PROFILE MANAGER] Durum gecmisi eklendi: " << abstract_state_to_string(state) << "\n";
    }
    // Base class'ta virtual olmadığı için 'override' kaldırıldı
    void add_explicit_action_feedback(UserIntent intent, AIAction action, bool approved) /*override*/ { 
        (void)intent; (void)action; (void)approved; 
        // std::cout << "[DUMMY USER PROFILE MANAGER] Acik eylem geri bildirimi kaydedildi.\n";
    }
    // Base class'ta virtual olmadığı için 'override' kaldırıldı
    void load_profile(const std::string& filename) /*override*/ { 
        (void)filename; 
        // std::cout << "[DUMMY USER PROFILE MANAGER] Profil yuklendi: " << filename << "\n";
    }
    // Base class'ta virtual olmadığı için 'override' kaldırıldı
    void save_profile(const std::string& filename) const /*override*/ { 
        (void)filename; 
        // std::cout << "[DUMMY USER PROFILE MANAGER] Profil kaydedildi: " << filename << "\n";
    }
};

// Dummy IntentLearner
class DummyIntentLearner : public IntentLearner {
public:
    DummyIntentLearner(IntentAnalyzer& analyzer_ref, SuggestionEngine& suggester_ref, UserProfileManager& user_profile_manager_ref)
        : IntentLearner(analyzer_ref, suggester_ref, user_profile_manager_ref) {}

    // process_explicit_feedback base class'ta virtual olduğu için 'override' anahtar kelimesi korundu
    virtual void process_explicit_feedback(UserIntent predicted_intent, AIAction action, bool approved, const DynamicSequence& sequence, AbstractState current_abstract_state) override {
        (void)sequence; 
        std::cout << "[DUMMY LEARNER] Acik geri bildirim: " << intent_to_string(predicted_intent)
             << ", eylem=" << static_cast<int>(action) << ", onay=" << (approved ? "Evet" : "Hayir")
             << ", durum=" << abstract_state_to_string(current_abstract_state) << "\n";
    }

    // infer_abstract_state base class'ta virtual olduğu için 'override' anahtar kelimesi korundu
    virtual AbstractState infer_abstract_state(const std::deque<AtomicSignal>& recent_signals) override {
        if (recent_signals.size() > 50) return AbstractState::HighProductivity;
        if (recent_signals.empty()) return AbstractState::None;
        return AbstractState::NormalOperation;
    }
};

// Dummy PredictionEngine
class DummyPredictionEngine : public PredictionEngine {
public:
    DummyPredictionEngine(IntentAnalyzer& analyzer_ref, SequenceManager& sequence_manager_ref)
        : PredictionEngine(analyzer_ref, sequence_manager_ref) {}
    // predict_next_intent base class'ta virtual olduğu için 'override' anahtar kelimesi korundu
    virtual UserIntent predict_next_intent(UserIntent current_intent, const DynamicSequence& sequence) const override { 
        (void)current_intent; (void)sequence; 
        return UserIntent::Unknown;
    }
    // Base class'ta virtual olmadığı için 'override' kaldırıldı
    void update_state_graph(UserIntent from_intent, UserIntent to_intent, const DynamicSequence& sequence) /*override*/ { 
        (void)from_intent; (void)to_intent; (void)sequence; 
        // std::cout << "[DUMMY PREDICTOR] Durum grafi guncellendi.\n";
    }
    // Base class'ta virtual olmadığı için 'override' kaldırıldı
    void save_state_graph(const std::string& filename) const /*override*/ { 
        (void)filename; 
        // std::cout << "[DUMMY PREDICTOR] State graph kaydedildi: " << filename << "\n";
    }
    // Base class'ta virtual olmadığı için 'override' kaldırıldı
    void load_state_graph(const std::string& filename) /*override*/ { 
        (void)filename; 
        // std::cout << "[DUMMY PREDICTOR] State graph yuklendi: " << filename << "\n";
    }
};

// DummyCryptofigAutoencoder
class DummyCryptofigAutoencoder : public CryptofigAutoencoder {
public:
    DummyCryptofigAutoencoder() : CryptofigAutoencoder() {}
    // encode base class'ta virtual olduğu için 'override' anahtar kelimesi korundu
    virtual std::vector<float> encode(const std::vector<float>& input_features) const override {
        (void)input_features; 
        return {0.1f, 0.2f, 0.3f};
    }
    // Base class'ta virtual olmadığı için 'override' kaldırıldı
    void save_weights(const std::string& filename) const /*override*/ { 
        (void)filename; 
        // std::cout << "[DUMMY AUTOENCODER] Weights saved: " << filename << "\n";
    }
    // Base class'ta virtual olmadığı için 'override' kaldırıldı
    void load_weights(const std::string& filename) /*override*/ {  
        (void)filename; 
        // std::cout << "[DUMMY AUTOENCODER] Weights loaded: " << filename << "\n";
    }
    // calculate_reconstruction_error base class'ta virtual olduğu için 'override' anahtar kelimesi korundu
    virtual float calculate_reconstruction_error(const std::vector<float>& original, const std::vector<float>& reconstructed) const override {
        (void)original; (void)reconstructed; 
        return 0.0f;
    }
};

// DummyCryptofigProcessor
class DummyCryptofigProcessor : public CryptofigProcessor {
public:
    DummyCryptofigProcessor(IntentAnalyzer& analyzer_ref, CryptofigAutoencoder& autoencoder_ref)
        : CryptofigProcessor(analyzer_ref, autoencoder_ref) {}

    // Base class'ta virtual olmadığı için 'override' kaldırıldı
    void process_sequence(DynamicSequence& sequence, float autoencoder_learning_rate) /*override*/ { 
        (void)autoencoder_learning_rate; 
        // std::cout << "[DUMMY CRYPTOFIG PROCESSOR] process_sequence called.\n";
        sequence.latent_cryptofig_vector = {0.4f, 0.5f, 0.6f};
    }
    // process_expert_cryptofig base class'ta virtual olduğu için 'override' anahtar kelimesi korundu
    virtual void process_expert_cryptofig(const std::vector<float>& expert_cryptofig, IntentLearner& learner) override {
        (void)expert_cryptofig; (void)learner; 
        // std::cout << "[DUMMY CRYPTOFIG PROCESSOR] process_expert_cryptofig.\n";
    }
    // generate_cryptofig_from_signals base class'ta virtual olduğu için 'override' anahtar kelimesi korundu
    virtual std::vector<float> generate_cryptofig_from_signals(const DynamicSequence& sequence) override {
        (void)sequence; 
        return {0.7f, 0.8f, 0.9f};
    }
    // get_autoencoder (const) base class'ta virtual olduğu için 'override' anahtar kelimesi korundu
    virtual const CryptofigAutoencoder& get_autoencoder() const override {
        static DummyCryptofigAutoencoder da;
        return da;
    }
    // get_autoencoder (non-const) base class'ta virtual olduğu için 'override' anahtar kelimesi korundu
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

    // generate_insights base class'ta virtual olduğu için 'override' anahtar kelimesi korundu
    virtual std::vector<AIInsight> generate_insights(const DynamicSequence& sequence) override {
        (void)sequence; 
        // DÜZELTİLDİ: Constructor kullanıldı
        AIInsight insight("Performance looks good.", AIAction::None, 0.8f); 
        std::vector<AIInsight> out;
        out.push_back(insight);
        return out;
    }
};

// DummyGoalManager
class DummyGoalManager : public GoalManager {
public:
    DummyGoalManager(AIInsightsEngine& ie) : GoalManager(ie) {}
    // get_current_goal base class'ta virtual olduğu için 'override' anahtar kelimesi korundu
    virtual AIGoal get_current_goal() const override {
        return AIGoal::OptimizeProductivity;
    }
};

// DummyNaturalLanguageProcessor
class DummyNaturalLanguageProcessor : public NaturalLanguageProcessor {
public:
    // NaturalLanguageProcessor'ın kurucusunda GoalManager& gm aldığı için
    // parametresiz constructor'ı çağıramayız, ancak test için bir DummyGoalManager'a ihtiyacımız var.
    // DÜZELTİLDİ: DummyNaturalLanguageProcessor(GoalManager& gm) kurucusu kullanıldı.
    DummyNaturalLanguageProcessor(GoalManager& gm) : NaturalLanguageProcessor(gm) {} 

    // Base class'ta virtual olmadığı için 'override' kaldırıldı
    std::string generate_response_text(UserIntent current_intent, AbstractState current_abstract_state, AIGoal current_goal,
                                  const DynamicSequence& sequence, const std::vector<std::string>& relevant_keywords = {}) const /*override*/ { 
        (void)current_intent; (void)current_abstract_state; (void)current_goal; (void)sequence; (void)relevant_keywords; 
        return "NLP'den gelen test yaniti.";
    }
    // Base class'ta virtual olmadığı için 'override' kaldırıldı
    UserIntent infer_intent_from_text(const std::string& user_input) const /*override*/ { 
        if (user_input.find("oyun") != std::string::npos) return UserIntent::Gaming;
        return UserIntent::GeneralInquiry;
    }
    // Base class'ta virtual olmadığı için 'override' kaldırıldı
    AbstractState infer_state_from_text(const std::string& user_input) const /*override*/ { 
        if (user_input.find("pil") != std::string::npos) return AbstractState::PowerSaving;
        return AbstractState::NormalOperation;
    }
};

// YENİ: Dummy WebFetcher
class DummyWebFetcher : public WebFetcher {
public:
    DummyWebFetcher() : WebFetcher() {}
    // search base class'ta virtual olduğu için 'override' anahtar kelimesi korundu
    virtual std::vector<WebResult> search(const std::string& query) override { // DÜZELTİLDİ: WebResult ve override eklendi
        (void)query;
        // Basit bir test sonucu döndür
        WebResult res1 = {"Test Content 1", "Test Source 1"}; // DÜZELTİLDİ
        WebResult res2 = {"Test Content 2", "Test Source 2"}; // DÜZELTİLDİ
        return {res1, res2};
    }
};


// === Test main ===
int main() {
    std::cout << "ResponseEngine testleri (düzeltilmiş) başlatılıyor...\n";
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
    // DummyNaturalLanguageProcessor artık GoalManager& gm alıyor
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
    test_lm.learnFromText("Qt programlama cok keyifli.", "Blog", "Programlama"); // ASCII uyumlu
    test_lm.learnFromText("Qt tasarim prensipleri.", "Dokuman", "Programlama"); // ASCII uyumlu
    assert(test_kb.getCapsulesByTopic("Genel").size() == 1);
    assert(test_kb.getCapsulesByTopic("Programlama").size() == 2);
    std::cout << "learnFromText başarılı.\n";

    // Test KnowledgeBase::findSimilar
    std::cout << "\nKnowledgeBase::findSimilar testi:\n";
    auto similar_capsules = test_kb.findSimilar("Qt", 1);
    assert(!similar_capsules.empty());
    std::cout << "En benzer kapsül: " << similar_capsules[0].content << "\n";

    // Test KnowledgeBase::encrypt
    std::cout << "\nKnowledgeBase::encrypt testi:\n";
    std::string original_text = "Gizli bilgi";
    std::string encrypted_text = test_kb.encrypt(original_text);
    std::string decrypted_text = test_kb.encrypt(encrypted_text); 
    assert(original_text == decrypted_text);
    std::cout << "Şifreleme/çözme başarılı.\n";

    // Test LearningModule::process_ai_insights
    std::cout << "\nLearningModule::process_ai_insights testi:\n";
    std::vector<AIInsight> dummy_insights;
    dummy_insights.push_back(AIInsight("New performance improvement detected.", AIAction::None, 0.9f)); 
    dummy_insights.push_back(AIInsight("User interface response time decreased.", AIAction::None, 0.7f)); 
    test_lm.process_ai_insights(dummy_insights);
    assert(test_kb.getCapsulesByTopic("AI Insight").size() == dummy_insights.size()); 
    std::cout << "process_ai_insights başarılı.\n";

    std::cout << "\nTüm testler tamamlandi (düzeltilmiş test dosyası).\n";
    return 0;
}