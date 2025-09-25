#include "natural_language_processor.h"
#include "../core/logger.h"
#include "../core/utils.h" // intent_to_string, abstract_state_to_string, goal_to_string, action_to_string için
#include "../brain/autoencoder.h" // CryptofigAutoencoder için (INPUT_DIM, LATENT_DIM)
#include <cctype> // std::tolower için
#include <algorithm> // std::transform için
#include <random> // rastgele seçim için

namespace CerebrumLux { // TÜM İMPLEMENTASYON BU NAMESPACE İÇİNDE OLACAK

NaturalLanguageProcessor::NaturalLanguageProcessor(CerebrumLux::GoalManager& goal_manager_ref)
    : goal_manager(goal_manager_ref) {

    // Niyet anahtar kelime haritasını başlat
    this->intent_keyword_map[CerebrumLux::UserIntent::Programming] = {"kod", "compile", "derle", "debug", "hata", "function", "class", "stack"};
    this->intent_keyword_map[CerebrumLux::UserIntent::Gaming]      = {"oyun", "fps", "level", "play", "match", "steam"};
    this->intent_keyword_map[CerebrumLux::UserIntent::MediaConsumption] = {"video", "izle", "film", "müzik", "spotify", "youtube"};
    this->intent_keyword_map[CerebrumLux::UserIntent::CreativeWork] = {"tasarla", "foto", "görsel", "müzik", "compose", "yarat", "üret"};
    this->intent_keyword_map[CerebrumLux::UserIntent::Research]    = {"araştır", "search", "makale", "pdf", "doküman", "read", "oku"};
    this->intent_keyword_map[CerebrumLux::UserIntent::Communication] = {"mail", "mesaj", "sohbet", "reply", "gönder"};
    this->intent_keyword_map[CerebrumLux::UserIntent::Editing]     = {"düzenle", "edit", "revize", "fix", "format"};
    this->intent_keyword_map[CerebrumLux::UserIntent::FastTyping]  = {"hızlı", "yaz", "typing", "type", "speed"};
    this->intent_keyword_map[CerebrumLux::UserIntent::Question] = {"neden", "nasıl", "kim", "ne", "niçin", "soru", "öğren"};
    this->intent_keyword_map[CerebrumLux::UserIntent::Command] = {"yap", "başlat", "durdur", "sil", "oluştur", "git"};
    this->intent_keyword_map[CerebrumLux::UserIntent::Statement] = {"bilgi", "gerçek", "biliyorum", "düşünüyorum"};
    this->intent_keyword_map[CerebrumLux::UserIntent::FeedbackPositive] = {"iyi", "güzel", "teşekkürler", "harika"};
    this->intent_keyword_map[CerebrumLux::UserIntent::FeedbackNegative] = {"kötü", "hayır", "beğenmedim", "yanlış"};
    this->intent_keyword_map[CerebrumLux::UserIntent::Greeting] = {"merhaba", "selam", "hi", "günaydın"};
    this->intent_keyword_map[CerebrumLux::UserIntent::Farewell] = {"güle güle", "hoşça kal", "bay bay", "görüşürüz"};
    this->intent_keyword_map[CerebrumLux::UserIntent::RequestInformation] = {"ver", "göster", "bilgi"};
    this->intent_keyword_map[CerebrumLux::UserIntent::ExpressEmotion] = {"mutlu", "üzgün", "sinirli"};
    this->intent_keyword_map[CerebrumLux::UserIntent::Confirm] = {"evet", "tamam", "onayla"};
    this->intent_keyword_map[CerebrumLux::UserIntent::Deny] = {"hayır", "reddet"};
    this->intent_keyword_map[CerebrumLux::UserIntent::Elaborate] = {"açıkla", "detaylandır"};
    this->intent_keyword_map[CerebrumLux::UserIntent::Clarify] = {"anlamadım", "tekrarla"};
    this->intent_keyword_map[CerebrumLux::UserIntent::CorrectError] = {"düzelt", "hata", "yanlış"};
    this->intent_keyword_map[CerebrumLux::UserIntent::InquireCapability] = {"yapabilir misin", "yeteneğin ne"};
    this->intent_keyword_map[CerebrumLux::UserIntent::ShowStatus] = {"durum", "nedir"};
    this->intent_keyword_map[CerebrumLux::UserIntent::ExplainConcept] = {"anlamı ne", "nedir"};


    // Durum anahtar kelime haritasını başlat
    this->state_keyword_map[CerebrumLux::AbstractState::PowerSaving] = {"pil", "battery", "şarj", "charging", "battery low", "pil zayıf"};
    this->state_keyword_map[CerebrumLux::AbstractState::FaultyHardware] = {"donanım", "arızalı", "error", "çök", "crash", "bozul"};
    this->state_keyword_map[CerebrumLux::AbstractState::Distracted] = {"dikkat", "dikkatim", "dikkat dağılı", "notification"};
    this->state_keyword_map[CerebrumLux::AbstractState::Focused] = {"odak", "focus", "konsantre", "akış"};
    this->state_keyword_map[CerebrumLux::AbstractState::SeekingInformation] = {"ara", "google", "bilgi", "sorgu"};

    // NLP'nin dahili model ağırlıklarını başlat (CryptofigAutoencoder boyutuna göre)
    for (auto intent_pair : this->intent_keyword_map) {
        this->intent_cryptofig_weights[intent_pair.first].assign(CerebrumLux::CryptofigAutoencoder::LATENT_DIM, 0.0f);
        // Rastgele değerlerle başlatma
        for (size_t i = 0; i < CerebrumLux::CryptofigAutoencoder::LATENT_DIM; ++i) {
            this->intent_cryptofig_weights[intent_pair.first][i] = static_cast<float>(CerebrumLux::SafeRNG::get_instance().get_generator()()) / CerebrumLux::SafeRNG::get_instance().get_generator().max();
        }
    }

    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "NaturalLanguageProcessor: Initialized."); // LogLevel namespace ile
}

// Metin tabanlı niyeti çıkarır
CerebrumLux::UserIntent NaturalLanguageProcessor::infer_intent_from_text(const std::string& user_input) const {
    std::string lower_text = user_input;
    std::transform(lower_text.begin(), lower_text.end(), lower_text.begin(),
                   [](unsigned char c){ return std::tolower(c); });

    // Kural tabanlı tahmin
    CerebrumLux::UserIntent guessed_intent = rule_based_intent_guess(lower_text);
    if (guessed_intent != CerebrumLux::UserIntent::Undefined) {
        return guessed_intent;
    }

    // Daha karmaşık modeller veya öğrenilmiş modeller burada kullanılabilir
    return CerebrumLux::UserIntent::Undefined;
}

// Metin tabanlı durumu çıkarır
CerebrumLux::AbstractState NaturalLanguageProcessor::infer_state_from_text(const std::string& user_input) const {
    std::string lower_text = user_input;
    std::transform(lower_text.begin(), lower_text.end(), lower_text.begin(),
                   [](unsigned char c){ return std::tolower(c); });

    CerebrumLux::AbstractState guessed_state = rule_based_state_guess(lower_text);
    if (guessed_state != CerebrumLux::AbstractState::Idle) {
        return guessed_state;
    }

    return CerebrumLux::AbstractState::Idle;
}

// Niyet ve latent kriptofig arasındaki skoru hesaplar
float NaturalLanguageProcessor::cryptofig_score_for_intent(CerebrumLux::UserIntent intent, const std::vector<float>& latent_cryptofig) const {
    auto it = this->intent_cryptofig_weights.find(intent);
    if (it == this->intent_cryptofig_weights.end() || latent_cryptofig.empty()) {
        return 0.0f;
    }

    const std::vector<float>& weights = it->second;
    if (weights.size() != latent_cryptofig.size()) {
        LOG_DEFAULT(CerebrumLux::LogLevel::WARNING, "NLP: cryptofig_score_for_intent - Ağırlık ve latent kriptofig boyutu uyuşmuyor.");
        return 0.0f;
    }

    float dot_product = 0.0f;
    for (size_t i = 0; i < weights.size(); ++i) {
        dot_product += weights[i] * latent_cryptofig[i];
    }
    // Basitçe dot product döndür, daha sonra sigmoid veya başka bir aktivasyon eklenebilir
    return dot_product;
}


// NLP'nin yanıt üretimi için merkezi metot
std::string NaturalLanguageProcessor::generate_response_text(
    CerebrumLux::UserIntent current_intent,
    CerebrumLux::AbstractState current_abstract_state,
    CerebrumLux::AIGoal current_goal,
    const CerebrumLux::DynamicSequence& sequence,
    const std::vector<std::string>& relevant_keywords
) const {
    std::string response = "Anladım. ";

    // Niyetlere göre yanıtlar
    if (current_intent == CerebrumLux::UserIntent::Question) {
        response += "Sorunuzu yanıtlamaya çalışıyorum. ";
    } else if (current_intent == CerebrumLux::UserIntent::Command) {
        response += "Komutunuzu işliyorum. ";
    } else if (current_intent == CerebrumLux::UserIntent::Greeting) {
        response = "Merhaba! Size nasıl yardımcı olabilirim? ";
    } else if (current_intent == CerebrumLux::UserIntent::FastTyping) {
        response = "Çok hızlı yazıyorsunuz! ";
    }

    // Durumlara göre yanıtlar
    if (current_abstract_state == CerebrumLux::AbstractState::Error) {
        response += "Bir hata durumu tespit ettim. ";
    } else if (current_abstract_state == CerebrumLux::AbstractState::Learning) {
        response += "Şu an öğreniyorum. ";
    }

    // Hedeflere göre yanıtlar
    if (current_goal == CerebrumLux::AIGoal::EnsureSecurity) {
        response += "Güvenliğinizi sağlamak için çalışıyorum. ";
    }

    // Anahtar kelimeler ve bağlam
    if (!relevant_keywords.empty()) {
        response += "Anahtar kelimeler: ";
        for (const auto& kw : relevant_keywords) {
            response += kw + ", ";
        }
        response = response.substr(0, response.length() - 2) + ". ";
    }

    // Eğer çok spesifik bir yanıt yoksa, fallback kullan
    if (response == "Anladım. ") {
        response = fallback_response_for_intent(current_intent, current_abstract_state, sequence);
    }
    return response;
}


// Kural tabanlı niyet tahmini
CerebrumLux::UserIntent NaturalLanguageProcessor::rule_based_intent_guess(const std::string& lower_text) const {
    for (const auto& pair : this->intent_keyword_map) {
        for (const auto& keyword : pair.second) {
            if (lower_text.find(keyword) != std::string::npos) {
                return pair.first;
            }
        }
    }
    return CerebrumLux::UserIntent::Undefined;
}

// Kural tabanlı durum tahmini
CerebrumLux::AbstractState NaturalLanguageProcessor::rule_based_state_guess(const std::string& lower_text) const {
    for (const auto& pair : this->state_keyword_map) {
        for (const auto& keyword : pair.second) {
            if (lower_text.find(keyword) != std::string::npos) {
                return pair.first;
            }
        }
    }
    return CerebrumLux::AbstractState::Idle;
}

// Model ağırlıklarını günceller
void NaturalLanguageProcessor::update_model(const std::string& observed_text, CerebrumLux::UserIntent true_intent, const std::vector<float>& latent_cryptofig) {
    if (latent_cryptofig.empty() || latent_cryptofig.size() != CerebrumLux::CryptofigAutoencoder::LATENT_DIM) {
        LOG_DEFAULT(CerebrumLux::LogLevel::WARNING, "NLP::update_model: Gecersiz latent kriptofig boyutu. Model guncellenemedi.");
        return;
    }

    // İlgili niyet için ağırlıkları bul veya oluştur
    auto& weights = this->intent_cryptofig_weights[true_intent];
    if (weights.empty()) {
        weights.assign(CerebrumLux::CryptofigAutoencoder::LATENT_DIM, 0.0f);
    }

    // Basit bir öğrenme kuralı: latent kriptofigi ağırlıklara dahil et
    float learning_rate_nlp = 0.1f;
    for (size_t i = 0; i < latent_cryptofig.size(); ++i) {
        weights[i] += learning_rate_nlp * (latent_cryptofig[i] - weights[i]);
        // Ağırlıkları belirli bir aralıkta tut (örneğin -10.0f ile 10.0f arası)
        weights[i] = std::min(10.0f, std::max(-10.0f, weights[i]));
    }
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "NLP::update_model updated weights for intent " << CerebrumLux::intent_to_string(true_intent));
}

// Modeli artımlı olarak eğitir
void NaturalLanguageProcessor::trainIncremental(const std::string& input, const std::string& expected_intent) {
    // Bu metod daha çok bir dış eğiticiden gelen veriyi işlemek için kullanılacaktır.
    // Metin girdisini işleyip, beklenen niyetle model ağırlıklarını güncelle.
    std::string lower_input = input;
    std::transform(lower_input.begin(), lower_input.end(), lower_input.begin(),
                   [](unsigned char c){ return std::tolower(c); });

    CerebrumLux::UserIntent true_intent = CerebrumLux::UserIntent::Undefined;
    if (expected_intent == "Programming") true_intent = CerebrumLux::UserIntent::Programming;
    else if (expected_intent == "Gaming") true_intent = CerebrumLux::UserIntent::Gaming;
    else if (expected_intent == "MediaConsumption") true_intent = CerebrumLux::UserIntent::MediaConsumption;
    else if (expected_intent == "CreativeWork") true_intent = CerebrumLux::UserIntent::CreativeWork;
    else if (expected_intent == "Research") true_intent = CerebrumLux::UserIntent::Research;
    else if (expected_intent == "Communication") true_intent = CerebrumLux::UserIntent::Communication;
    else if (expected_intent == "Editing") true_intent = CerebrumLux::UserIntent::Editing;
    else if (expected_intent == "FastTyping") true_intent = CerebrumLux::UserIntent::FastTyping;
    else if (expected_intent == "Question") true_intent = CerebrumLux::UserIntent::Question;
    else if (expected_intent == "Command") true_intent = CerebrumLux::UserIntent::Command;
    else if (expected_intent == "Statement") true_intent = CerebrumLux::UserIntent::Statement;
    else if (expected_intent == "FeedbackPositive") true_intent = CerebrumLux::UserIntent::FeedbackPositive;
    else if (expected_intent == "FeedbackNegative") true_intent = CerebrumLux::UserIntent::FeedbackNegative;
    else if (expected_intent == "Greeting") true_intent = CerebrumLux::UserIntent::Greeting;
    else if (expected_intent == "Farewell") true_intent = CerebrumLux::UserIntent::Farewell;
    else if (expected_intent == "RequestInformation") true_intent = CerebrumLux::UserIntent::RequestInformation;
    else if (expected_intent == "ExpressEmotion") true_intent = CerebrumLux::UserIntent::ExpressEmotion;
    else if (expected_intent == "Confirm") true_intent = CerebrumLux::UserIntent::Confirm;
    else if (expected_intent == "Deny") true_intent = CerebrumLux::UserIntent::Deny;
    else if (expected_intent == "Elaborate") true_intent = CerebrumLux::UserIntent::Elaborate;
    else if (expected_intent == "Clarify") true_intent = CerebrumLux::UserIntent::Clarify;
    else if (expected_intent == "CorrectError") true_intent = CerebrumLux::UserIntent::CorrectError;
    else if (expected_intent == "InquireCapability") true_intent = CerebrumLux::UserIntent::InquireCapability;
    else if (expected_intent == "ShowStatus") true_intent = CerebrumLux::UserIntent::ShowStatus;
    else if (expected_intent == "ExplainConcept") true_intent = CerebrumLux::UserIntent::ExplainConcept;


    if (true_intent == CerebrumLux::UserIntent::Undefined) {
        LOG_DEFAULT(CerebrumLux::LogLevel::WARNING, "NLP::trainIncremental: Bilinmeyen niyet: " << expected_intent << ". Öğrenme atlandı.");
        return;
    }

    // Geçici bir latent kriptofig oluştur (gerçekte Autoencoder'dan gelir)
    std::vector<float> dummy_cryptofig(CerebrumLux::CryptofigAutoencoder::LATENT_DIM, 0.5f);
    
    update_model(input, true_intent, dummy_cryptofig);
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "NLP::trainIncremental: '" << expected_intent << "' niyeti için artımlı öğrenme tamamlandı.");
}

// Modeli yükleme
void NaturalLanguageProcessor::load_model(const std::string& path) {
    // Model yükleme lojiği burada olacak (örneğin JSON veya binary format)
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "NLP: Model yüklendi (placeholder): " << path);
}

// Modeli kaydetme
void NaturalLanguageProcessor::save_model(const std::string& path) const {
    // Model kaydetme lojiği burada olacak
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "NLP: Model kaydedildi (placeholder): " << path);
}

// Geri dönüş yanıtı (fallback)
std::string NaturalLanguageProcessor::fallback_response_for_intent(CerebrumLux::UserIntent intent, CerebrumLux::AbstractState state, const CerebrumLux::DynamicSequence& sequence) const {
    // Daha akıllı, duruma duyarlı fallback yanıtlar
    std::string response = "Bu konuda emin değilim. ";
    if (intent == CerebrumLux::UserIntent::Undefined) {
        response += "Niyetinizi tam olarak anlayamadım.";
    } else {
        response += "Niyetiniz '" + CerebrumLux::intent_to_string(intent) + "' gibi görünüyor, ancak daha fazla bilgiye ihtiyacım var.";
    }
    return response;
}

} // namespace CerebrumLux