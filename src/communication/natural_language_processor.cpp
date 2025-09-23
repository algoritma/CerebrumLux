#include "natural_language_processor.h"
#include "../core/logger.h"
#include "../core/utils.h"
#include <sstream>
#include <algorithm>
#include <cmath>
#include <chrono>

// Default constructor for tooling
NaturalLanguageProcessor::NaturalLanguageProcessor()
    : goal_manager(nullptr) { 

    // intent keyword map (küçük ve dikkatli seçilmiş ilk set)
    intent_keyword_map[UserIntent::Programming] = {"kod", "compile", "derle", "debug", "hata", "function", "class", "stack"};
    intent_keyword_map[UserIntent::Gaming]      = {"oyun", "fps", "level", "play", "match", "steam"};
    intent_keyword_map[UserIntent::MediaConsumption] = {"video", "izle", "film", "müzik", "spotify", "youtube"};
    intent_keyword_map[UserIntent::CreativeWork] = {"tasarla", "foto", "görsel", "müzik", "compose", "yarat", "üret"};
    intent_keyword_map[UserIntent::Research]    = {"araştır", "search", "makale", "pdf", "doküman", "read", "oku"};
    intent_keyword_map[UserIntent::Communication] = {"mail", "mesaj", "sohbet", "reply", "gönder"};
    intent_keyword_map[UserIntent::Editing]     = {"düzenle", "edit", "revize", "fix", "format"};
    intent_keyword_map[UserIntent::FastTyping]  = {"hızlı", "yaz", "typing", "type", "speed"};

    // state keyword map
    state_keyword_map[AbstractState::PowerSaving] = {"pil", "battery", "şarj", "charging", "battery low", "pil zayıf"};
    state_keyword_map[AbstractState::FaultyHardware] = {"donanım", "arada", "error", "çök", "crash", "bozul"};
    state_keyword_map[AbstractState::Distracted] = {"dikkat", "dikkatim", "dikkat dağı", "uyarı", "notification"};
    state_keyword_map[AbstractState::Focused] = {"odak", "focus", "konsantre", "akış"};
}

// Constructor: initialize keyword maps
NaturalLanguageProcessor::NaturalLanguageProcessor(GoalManager& goal_manager_ref)
    : goal_manager(&goal_manager_ref) { 

    // intent keyword map (küçük ve dikkatli seçilmiş ilk set)
    intent_keyword_map[UserIntent::Programming] = {"kod", "compile", "derle", "debug", "hata", "function", "class", "stack"};
    intent_keyword_map[UserIntent::Gaming]      = {"oyun", "fps", "level", "play", "match", "steam"};
    intent_keyword_map[UserIntent::MediaConsumption] = {"video", "izle", "film", "müzik", "spotify", "youtube"};
    intent_keyword_map[UserIntent::CreativeWork] = {"tasarla", "foto", "görsel", "müzik", "compose", "yarat", "üret"};
    intent_keyword_map[UserIntent::Research]    = {"araştır", "search", "makale", "pdf", "doküman", "read", "oku"};
    intent_keyword_map[UserIntent::Communication] = {"mail", "mesaj", "sohbet", "reply", "gönder"};
    intent_keyword_map[UserIntent::Editing]     = {"düzenle", "edit", "revize", "fix", "format"};
    intent_keyword_map[UserIntent::FastTyping]  = {"hızlı", "yaz", "typing", "type", "speed"};

    // state keyword map
    state_keyword_map[AbstractState::PowerSaving] = {"pil", "battery", "şarj", "charging", "battery low", "pil zayıf"};
    state_keyword_map[AbstractState::FaultyHardware] = {"donanım", "arada", "error", "çök", "crash", "bozul"};
    state_keyword_map[AbstractState::Distracted] = {"dikkat", "dikkatim", "dikkat dağı", "uyarı", "notification"};
    state_keyword_map[AbstractState::Focused] = {"odak", "focus", "konsantre", "akış"};
}

// Helper: lowercase copy
std::string NaturalLanguageProcessor::to_lower_copy(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    return out;
}

// Helper: keyword contained
bool NaturalLanguageProcessor::contains_keyword(const std::string& lower_text, const std::string& lower_keyword) {
    if (lower_keyword.empty()) return false;
    return lower_text.find(lower_keyword) != std::string::npos;
}

// Rule-based guess for intent
UserIntent NaturalLanguageProcessor::rule_based_intent_guess(const std::string& lower_text) const {
    for (const auto& kv : intent_keyword_map) {
        for (const auto& kw : kv.second) {
            if (contains_keyword(lower_text, to_lower_copy(kw))) {
                return kv.first;
            }
        }
    }
    return UserIntent::Unknown;
}

// Rule-based guess for state
AbstractState NaturalLanguageProcessor::rule_based_state_guess(const std::string& lower_text) const {
    for (const auto& kv : state_keyword_map) {
        for (const auto& kw : kv.second) {
            if (contains_keyword(lower_text, to_lower_copy(kw))) {
                return kv.first;
            }
        }
    }
    return AbstractState::NormalOperation;
}

// Cryptofig scoring
float NaturalLanguageProcessor::cryptofig_score_for_intent(UserIntent intent, const std::vector<float>& latent_cryptofig) const {
    if (latent_cryptofig.empty()) return 0.0f;
    std::lock_guard<std::mutex> lock(model_mutex);
    auto it = intent_cryptofig_weights.find(intent);
    if (it == intent_cryptofig_weights.end()) return 0.0f;

    const std::vector<float>& w = it->second;
    size_t n = std::min(w.size(), latent_cryptofig.size());
    double dot = 0.0, wn = 0.0;
    for (size_t i = 0; i < n; ++i) {
        dot += w[i] * latent_cryptofig[i];
        wn += std::abs(w[i]);
    }
    if (wn == 0.0) return static_cast<float>(dot);
    return static_cast<float>(dot / (wn + 1e-6));
}

// Infer intent
UserIntent NaturalLanguageProcessor::infer_intent_from_text(const std::string& user_input) const {
    std::string lower = to_lower_copy(user_input);

    UserIntent rule_guess = rule_based_intent_guess(lower);
    if (rule_guess != UserIntent::Unknown) {
        LOG_DEFAULT(LogLevel::DEBUG, "NLP: rule_based intent guess = " << intent_to_string(rule_guess));
        return rule_guess;
    }

    if (contains_keyword(lower, "nasıl") || contains_keyword(lower, "ne") ||
        contains_keyword(lower, "neden") || contains_keyword(lower, "how") ||
        contains_keyword(lower, "what") || contains_keyword(lower, "why")) {
        return UserIntent::GeneralInquiry;
    }

    return UserIntent::Unknown;
}

// Infer state
AbstractState NaturalLanguageProcessor::infer_state_from_text(const std::string& user_input) const {
    std::string lower = to_lower_copy(user_input);
    AbstractState state = rule_based_state_guess(lower);
    LOG_DEFAULT(LogLevel::DEBUG, "NLP: rule_based state guess = " << abstract_state_to_string(state));
    return state;
}

// Generate response text
std::string NaturalLanguageProcessor::generate_response_text(
    UserIntent current_intent,
    AbstractState current_abstract_state,
    AIGoal current_goal,
    const DynamicSequence& sequence,
    const std::vector<std::string>& relevant_keywords
) const {
    std::stringstream ss;
    if (goal_manager) { 
        AIGoal gm_goal = goal_manager->get_current_goal();
        if (gm_goal != AIGoal::None && gm_goal != current_goal) {
            ss << "Benim güncel hedefim: " << goal_to_string(gm_goal) << ". ";
        }
    }

    switch (current_intent) {
        case UserIntent::Programming:
            ss << (current_abstract_state == AbstractState::Focused ?
                "Kod yazıyorsunuz gibi görünüyorsunuz. Yardım ister misiniz?" :
                "Programlama ile ilgili bir şey mi arıyorsunuz?"); break;
        case UserIntent::Gaming: ss << "Oyun modu algılandı. Bildirimleri sessize alabiliriz."; break;
        case UserIntent::MediaConsumption: ss << "Medya tüketimi modu. Rahatsız etmeyecek şekilde ortamı optimize edebilirim."; break;
        case UserIntent::CreativeWork: ss << "Yaratıcı çalışma algılandı. Odak akışınızı bozmayacak şekilde yardım ister misiniz?"; break;
        case UserIntent::Research: ss << "Araştırma modunda gibi görünüyorsunuz. İlgili kaynakları hızlıca getirebilirim."; break;
        case UserIntent::Communication: ss << "İletişim modunda. Yazım denetimi ve hızlı şablonlar sunabilirim."; break;
        case UserIntent::Editing: ss << "Düzenleme yapıyorsunuz. Geri al geçmişini veya biçimlendirme önerilerini gösterebilirim."; break;
        case UserIntent::FastTyping: ss << "Hızlı yazım modu algılandı. Otomatik düzeltme ve hızlı destek sunabilirim."; break;
        case UserIntent::GeneralInquiry: ss << "Sormak istediğiniz konuda yardımcı olabilirim. Sorunuzu detaylandırır mısınız?"; break;
        case UserIntent::Unknown: default:
            if (!relevant_keywords.empty()) ss << relevant_keywords.front() << " ";
            ss << "Anladığım kadarıyla niyet net değil. Daha fazla bilgi verir misiniz?"; break;
    }

    if (sequence.current_battery_percentage < 20 && !sequence.current_battery_charging)
        ss << " Pil durumu düşük (" << static_cast<int>(sequence.current_battery_percentage) << "%).";
    else if (sequence.current_battery_percentage < 45)
        ss << " Pil seviyesi orta. Uzun kullanım için pil önerileri sunabilirim.";

    LOG_DEFAULT(LogLevel::DEBUG, "NLP::generate_response_text -> " << ss.str());
    return ss.str();
}

// Online model update
void NaturalLanguageProcessor::update_model(const std::string& observed_text, UserIntent true_intent, const std::vector<float>& latent_cryptofig) {
    if (latent_cryptofig.empty()) return;
    std::lock_guard<std::mutex> lock(model_mutex);

    auto &weights = intent_cryptofig_weights[true_intent];
    if (weights.size() < latent_cryptofig.size()) weights.resize(latent_cryptofig.size(), 0.01f);

    float pred = 0.0f;
    for (size_t i = 0; i < latent_cryptofig.size(); ++i) pred += weights[i] * latent_cryptofig[i];

    float error = 1.0f - pred;
    for (size_t i = 0; i < latent_cryptofig.size(); ++i) weights[i] += online_learning_rate * error * latent_cryptofig[i];

    LOG_DEFAULT(LogLevel::DEBUG, "NLP::update_model updated weights for intent " << intent_to_string(true_intent));
}

// 🔹 Incremental training
void NaturalLanguageProcessor::trainIncremental(const std::string& input, const std::string& expected_intent) {
    UserIntent true_intent = UserIntent::Unknown;

    if (expected_intent == "Programming") true_intent = UserIntent::Programming;
    else if (expected_intent == "Gaming") true_intent = UserIntent::Gaming;
    else if (expected_intent == "MediaConsumption") true_intent = UserIntent::MediaConsumption;
    else if (expected_intent == "CreativeWork") true_intent = UserIntent::CreativeWork;
    else if (expected_intent == "Research") true_intent = UserIntent::Research;
    else if (expected_intent == "Communication") true_intent = UserIntent::Communication;
    else if (expected_intent == "Editing") true_intent = UserIntent::Editing;
    else if (expected_intent == "FastTyping") true_intent = UserIntent::FastTyping;
    else if (expected_intent == "GeneralInquiry") true_intent = UserIntent::GeneralInquiry;

    // latent_cryptofig normalde DynamicSequence'den gelmeli, burada bir dummy oluşturalım
    std::vector<float> dummy_cryptofig(CryptofigAutoencoder::LATENT_DIM, 0.5f); 
    update_model(input, true_intent, dummy_cryptofig);
}

// YENİ: load_model implementasyonu
void NaturalLanguageProcessor::load_model(const std::string& path) {
    std::lock_guard<std::mutex> lock(model_mutex);
    LOG_DEFAULT(LogLevel::INFO, "NLP: Model yüklendi (placeholder): " << path);
    // TODO: Gerçek model yükleme mantığı buraya gelecek
    // Örneğin, intent_cryptofig_weights haritasını dosyadan yükle
}

// YENİ: save_model implementasyonu
void NaturalLanguageProcessor::save_model(const std::string& path) const {
    std::lock_guard<std::mutex> lock(model_mutex);
    LOG_DEFAULT(LogLevel::INFO, "NLP: Model kaydedildi (placeholder): " << path);
    // TODO: Gerçek model kaydetme mantığı buraya gelecek
    // Örneğin, intent_cryptofig_weights haritasını dosyaya kaydet
}

// YENİ: predict_intent implementasyonu
std::string NaturalLanguageProcessor::predict_intent(const std::string& input) {
    std::string lower = to_lower_copy(input);
    UserIntent intent = infer_intent_from_text(lower); // Mevcut kural tabanlı çıkarım
    
    // Daha sofistike bir tahmin için intent_cryptofig_weights kullanılabilir.
    // Şimdilik sadece kural tabanlı tahmini döndürüyoruz.
    LOG_DEFAULT(LogLevel::INFO, "NLP: Tahmin edilen intent (placeholder): " << intent_to_string(intent));
    return intent_to_string(intent);
}

// YENİ: fallback_response_for_intent implementasyonu
std::string NaturalLanguageProcessor::fallback_response_for_intent(UserIntent intent, AbstractState state, const DynamicSequence& sequence) const {
    (void)state; // Kullanılmadı
    (void)sequence; // Kullanılmadı
    return "Anladığım kadarıyla niyetiniz: " + intent_to_string(intent) + ". Size nasıl yardımcı olabilirim?";
}