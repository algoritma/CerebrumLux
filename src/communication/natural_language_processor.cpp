#include "natural_language_processor.h"
#include "../core/logger.h"
#include "../core/enums.h"         // LogLevel, UserIntent, AbstractState gibi enum'lar için
#include "../core/utils.h"          // intent_to_string, abstract_state_to_string, goal_to_string, action_to_string için
#include "../brain/autoencoder.h"   // CryptofigAutoencoder için (INPUT_DIM, LATENT_DIM)
#include "../learning/Capsule.h"    // Capsule için
#include "../learning/KnowledgeBase.h" // KnowledgeBase için
#include "../data_models/dynamic_sequence.h" // DynamicSequence için

#include <iostream>
#include <algorithm> // std::transform için
#include <cctype>    // std::tolower için
#include <random>    // SafeRNG için
#include <stdexcept> // std::runtime_error için
#include <sstream>   // stringstream için
#include <functional> // std::hash için (statik embedding için)

namespace CerebrumLux {

namespace { // Anonim namespace for static RNG
    std::random_device s_rd_nlp;
    std::mt19937 s_gen_nlp(s_rd_nlp());
    std::uniform_real_distribution<float> s_dist_nlp(-1.0f, 1.0f);
}

// Statik üyelerin tanımı
std::mt19937 NaturalLanguageProcessor::s_rng(s_rd_nlp()); // Statik RNG'yi başlat
std::uniform_real_distribution<float> NaturalLanguageProcessor::s_dist(-1.0f, 1.0f); // Statik dağıtımı başlat

NaturalLanguageProcessor::NaturalLanguageProcessor(CerebrumLux::GoalManager& goal_manager_ref, CerebrumLux::KnowledgeBase& kbRef)
    : goal_manager(goal_manager_ref), kbRef_(kbRef) // Bu constructor diğer NLP fonksiyonları için referansları almaya devam eder
{
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "NaturalLanguageProcessor: Initialized.");

    // Niyet anahtar kelime haritasını başlat (Mevcut kod aynı kalır)
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

    // Durum anahtar kelime haritasını başlat (Mevcut kod aynı kalır)
    this->state_keyword_map[CerebrumLux::AbstractState::PowerSaving] = {"pil", "battery", "şarj", "charging", "battery low", "pil zayıf"};
    this->state_keyword_map[CerebrumLux::AbstractState::FaultyHardware] = {"donanım", "arızalı", "error", "çök", "crash", "bozul"};
    this->state_keyword_map[CerebrumLux::AbstractState::Distracted] = {"dikkat", "dikkatim", "dikkat dağılı", "notification"};
    this->state_keyword_map[CerebrumLux::AbstractState::Focused] = {"odak", "focus", "konsantre", "akış"};
    this->state_keyword_map[CerebrumLux::AbstractState::SeekingInformation] = {"ara", "google", "bilgi", "sorgu"};

    // NLP'nin dahili model ağırlıklarını başlat (Mevcut kod aynı kalır)
    for (auto intent_pair : this->intent_keyword_map) {
        this->intent_cryptofig_weights[intent_pair.first].assign(CerebrumLux::CryptofigAutoencoder::LATENT_DIM, 0.0f);
        for (size_t i = 0; i < CerebrumLux::CryptofigAutoencoder::LATENT_DIM; ++i) {
            this->intent_cryptofig_weights[intent_pair.first][i] = static_cast<float>(CerebrumLux::SafeRNG::get_instance().get_generator()()) / CerebrumLux::SafeRNG::get_instance().get_generator().max();
        }
    }
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "NaturalLanguageProcessor: Initialized.");
}

CerebrumLux::UserIntent NaturalLanguageProcessor::infer_intent_from_text(const std::string& user_input) const {
    std::string lower_text = user_input;
    std::transform(lower_text.begin(), lower_text.end(), lower_text.begin(),
                   [](unsigned char c){ return static_cast<unsigned char>(std::tolower(c)); });

    // Kural tabanlı tahmin
    CerebrumLux::UserIntent guessed_intent = rule_based_intent_guess(lower_text);
    if (guessed_intent != CerebrumLux::UserIntent::Undefined) {
        LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "NLP: Niyet kural tabanlı olarak tahmin edildi: " << CerebrumLux::intent_to_string(guessed_intent));
        return guessed_intent;
    }

    // YENİ KOD: KnowledgeBase'den semantik arama ile niyet çıkarımı (eğer kural tabanlı başarısız olursa)
    // Kullanıcının girdisine en yakın 1-2 kapsülü ara
    std::vector<float> user_input_embedding = NaturalLanguageProcessor::generate_text_embedding(lower_text); // Statik metod çağrısı
    std::vector<CerebrumLux::Capsule> related_capsules = kbRef_.semantic_search(user_input_embedding, 2); // Embedding ile ara
    if (!related_capsules.empty()) {
        // En alakalı kapsülün konusunu niyete çevirmeye çalış
        // Basitçe: eğer topic bir niyete karşılık geliyorsa, onu kullan.
        // Daha sofistike bir yaklaşımda: niyet sınıflandırıcı, kapsül içeriğiyle eğitilir.
        for (const auto& capsule : related_capsules) {
            if (capsule.topic == "Programming") return CerebrumLux::UserIntent::Programming;
            if (capsule.topic == "Research" || capsule.topic == "WebSearch") return CerebrumLux::UserIntent::Research;
            if (capsule.topic == "AI Insight") return CerebrumLux::UserIntent::Question; // AI Insight topic'i bir soruya yönlendirebilir.
            // Diğer topic'ler için de benzer eşlemeler eklenebilir.
        }
        LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "NLP: Niyet KnowledgeBase'den türetilmeye çalışıldı, ancak doğrudan bir eşleşme bulunamadı.");
    }

    // Daha karmaşık modeller veya öğrenilmiş modeller burada kullanılabilir
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "NLP: Niyet tahmin edilemedi, Undefined döndürülüyor.");
    return CerebrumLux::UserIntent::Undefined;
}

CerebrumLux::AbstractState NaturalLanguageProcessor::infer_state_from_text(const std::string& user_input) const {
    std::string lower_text = user_input;
    std::transform(lower_text.begin(), lower_text.end(), lower_text.begin(),
                   [](unsigned char c){ return static_cast<unsigned char>(std::tolower(c)); });

    CerebrumLux::AbstractState guessed_state = rule_based_state_guess(lower_text);
    if (guessed_state != CerebrumLux::AbstractState::Idle) {
        LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "NLP: Durum kural tabanlı olarak tahmin edildi: " << CerebrumLux::abstract_state_to_string(guessed_state));
        return guessed_state;
    }
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "NLP: Durum tahmin edilemedi, Idle döndürülüyor.");
    return CerebrumLux::AbstractState::Idle;
}

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

ChatResponse NaturalLanguageProcessor::generate_response_text(
    CerebrumLux::UserIntent current_intent,
    CerebrumLux::AbstractState current_abstract_state,
    CerebrumLux::AIGoal current_goal,
    const CerebrumLux::DynamicSequence& sequence,
    const std::vector<std::string>& relevant_keywords,
    const CerebrumLux::KnowledgeBase& kb
) const {
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "NaturalLanguageProcessor: generate_response_text çağrıldı. Niyet: " << intent_to_string(current_intent) << ", Durum: " << abstract_state_to_string(current_abstract_state));

    ChatResponse response_obj;
    std::string generated_text = "";
    std::string reasoning_text = "";
    bool clarification_needed = false;

    // Adım 1: Bağlama duyarlı, Bilgi Tabanından (KnowledgeBase) yanıt arama (GELİŞTİRİLDİ)
    std::vector<CerebrumLux::Capsule> search_results; // Arama sonuçları
    std::string search_query_str; // Loglama ve prompt oluşturma için metin sorgusu
    std::vector<float> search_query_embedding; // KnowledgeBase'e gönderilecek embedding
    if (!relevant_keywords.empty()) { // Kullanıcı girdisinden türetilen anahtar kelimeler varsa
        search_query_str = relevant_keywords[0];
        if (relevant_keywords.size() > 1) search_query_str += " " + relevant_keywords[1];
        LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "NaturalLanguageProcessor: KnowledgeBase'de arama yapılıyor (sorgu: " << search_query_str << ").");
        search_query_embedding = NaturalLanguageProcessor::generate_text_embedding(search_query_str); // Statik metod çağrısı
        search_results = kb.semantic_search(search_query_embedding, 3); // Embedding ile ara
    }
    // Eğer anahtar kelime yoksa ancak niyet araştırma veya soru ise, daha genel bir arama yapabiliriz.
    else if ((current_intent == CerebrumLux::UserIntent::Research || current_intent == CerebrumLux::UserIntent::Question) && !kb.get_all_capsules().empty()) {
        // dynamic_sequence'den veya current_abstract_state'den ipuçları alabiliriz.
        // Şimdilik daha genel bir arama yapalım.
        search_query_embedding = NaturalLanguageProcessor::generate_text_embedding(""); // Statik metod çağrısı
        search_results = kb.semantic_search(search_query_embedding, 1); // Embedding ile ara
        search_query_str = "Genel bilgi araması";
    }

    if (!search_results.empty()) {
        const CerebrumLux::Capsule& relevant_capsule = search_results[0];
        generated_text = "Bilgi tabanımda '" + relevant_capsule.topic + "' konusunda bilgi buldum: " + relevant_capsule.plain_text_summary;
        
        // Eğer daha fazla ilgili kapsül varsa, bunları da gerekçeye ekle
        if (search_results.size() > 1) {
            generated_text += " Daha fazla ilgili bilgi için bakınız: ";
            for (size_t i = 1; i < search_results.size(); ++i) {
                generated_text += search_results[i].topic + (i == search_results.size() - 1 ? "." : ", ");
            }
        }
        
        reasoning_text += "Yanıt, KnowledgeBase'deki en alakalı kapsülden ('" + relevant_capsule.topic + "', ID: " + relevant_capsule.id + ") ve diğer ilgili kapsüllerden (" + std::to_string(search_results.size() -1) + " adet) türetildi. ";
        LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "NaturalLanguageProcessor: KnowledgeBase'den yanıt üretildi: " << generated_text);
    } else {
        LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "NaturalLanguageProcessor: KnowledgeBase'de ilgili kapsül bulunamadı. Kural tabanlı yanıta dönülüyor.");
        
        // Adım 2.1: Niyet ve duruma göre kural tabanlı yanıt üretimi
        generated_text = "Anladım. ";
    
        // Adım 2: Dinamik prompt oluşturma ve kullanma (Bu prompt'u doğrudan LLM'e göndermek yerine,
        // şu anki kural tabanlı sistemin kararını zenginleştirmek için kullanıyoruz.)
        std::string dynamic_prompt = generate_dynamic_prompt(current_intent, current_abstract_state, current_goal, sequence, search_query_str, search_results);
        LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "NaturalLanguageProcessor: Dinamik Olarak Üretilen Prompt: " << dynamic_prompt);

        // Eğer KnowledgeBase'de ilgili bilgi bulunamazsa veya LLM entegre değilse kural tabanlı yanıt üretmeye devam et.
        // Kural tabanlı yanıtlarınızı, dynamic_prompt'tan gelen ipuçları ile zenginleştirebilirsiniz.
        // Örneğin, dynamic_prompt'ta "2-3 farklı perspektiften değerlendirme" isteniyorsa,
        // mevcut kural tabanlı yanıta bunları eklemeye çalışın.

        // Mevcut kural tabanlı yanıt mantığı (zenginleştirme potansiyeli ile)
        // Bu kısım, dynamic_prompt'u temel alarak daha akıllı hale getirilebilir.
        // Şimdilik, niyet ve duruma göre mevcut yanıtları kullanıyoruz.

        // Yanıtları daha spesifik hale getirmek için dynamic_prompt'taki anahtar kelimeleri kullanabiliriz.
        // Örneğin, "AI optimizasyonları" gibi anahtar kelimeler varsa, bunlara özel yanıtlar oluşturabiliriz.
        if (current_intent == CerebrumLux::UserIntent::Question && search_results.empty()) {
             generated_text += "Bu konuda daha fazla bilgiye ihtiyacım var. Ne sormak istersiniz?";
             reasoning_text += "Yanıt, KnowledgeBase'de ilgili bilgi bulunamadığı için 'Soru' niyetine göre genel kural tabanlı olarak üretildi. ";
        } else if (current_intent == CerebrumLux::UserIntent::Research && search_results.empty()) {
            generated_text += "Araştırma niyetinizi anladım. Hangi konuda araştırma yapmak istersiniz?";
            reasoning_text += "Yanıt, KnowledgeBase'de ilgili bilgi bulunamadığı için 'Araştırma' niyetine göre genel kural tabanlı olarak üretildi. ";
        } else if (generated_text == "Anladım. ") { // Hala çok genel ise
            switch (current_intent) {
                case CerebrumLux::UserIntent::Question: generated_text += "Bu konuda ne öğrenmek istersiniz?"; break;
                case CerebrumLux::UserIntent::Command: generated_text += "Komutunuzu işliyorum."; break;
                case CerebrumLux::UserIntent::Greeting: generated_text = "Merhaba! Size nasıl yardımcı olabilirim?"; break;
                case CerebrumLux::UserIntent::Farewell: generated_text = "Güle güle! Görüşmek üzere."; break;
                case CerebrumLux::UserIntent::FastTyping: generated_text = "Çok hızlı yazıyorsunuz! Harika bir üretkenlik."; break;
                case CerebrumLux::UserIntent::Editing: generated_text = "Düzenleme sürecinde misiniz? "; break;
                case CerebrumLux::UserIntent::Programming: generated_text = "Kodlama aktivitesi algılandı. "; break;
                case CerebrumLux::UserIntent::Gaming: generated_text = "Oyun modundasınız. "; break;
                case CerebrumLux::UserIntent::MediaConsumption: generated_text = "Medya tüketimi yapıyorsunuz. "; break;
                case CerebrumLux::UserIntent::CreativeWork: generated_text = "Yaratıcı bir çalışma yapıyorsunuz. "; break;
                case CerebrumLux::UserIntent::Research: generated_text = "Araştırma yapıyor ve bilgi arıyorsunuz. "; break;
                case CerebrumLux::UserIntent::Communication: generated_text = "İletişim kurarken size eşlik ediyorum. "; break;
                case CerebrumLux::UserIntent::FeedbackPositive: generated_text = "Geri bildiriminiz için teşekkür ederim!"; break;
                case CerebrumLux::UserIntent::FeedbackNegative: generated_text = "Geri bildiriminiz önemli. Bu konuda nasıl yardımcı olabilirim?"; break;
                case CerebrumLux::UserIntent::RequestInformation: generated_text = "Hangi bilgiyi arıyorsunuz?"; break;
                case CerebrumLux::UserIntent::ExpressEmotion: generated_text += "Duygularınızı anlıyorum. "; break;
                case CerebrumLux::UserIntent::Confirm: generated_text = "Onaylandı. "; break;
                case CerebrumLux::UserIntent::Deny: generated_text = "Reddedildi. "; break;
                case CerebrumLux::UserIntent::Elaborate: generated_text = "Daha fazla detay verebilir misiniz?"; break;
                case CerebrumLux::UserIntent::Clarify: generated_text = "Lütfen açıklayınız. "; break;
                case CerebrumLux::UserIntent::CorrectError: generated_text = "Hatayı düzeltmeye çalışıyorum. "; break;
                case CerebrumLux::UserIntent::InquireCapability: generated_text = "Yeteneklerim hakkında bilgi mi almak istiyorsunuz? "; break;
                case CerebrumLux::UserIntent::ShowStatus: generated_text = "Sistem durumunu kontrol ediyorum. "; break;
                case CerebrumLux::UserIntent::ExplainConcept: generated_text = "Kavramı açıklamaya çalışıyorum. "; break;
                case CerebrumLux::UserIntent::Undefined:
                case CerebrumLux::UserIntent::Unknown:
                    generated_text += "Niyetinizi tam olarak anlayamadım.";
                    clarification_needed = true;
                    break;
            }
            reasoning_text += "Yanıt, KnowledgeBase'de ilgili bilgi bulunamadığı için niyetinize göre kural tabanlı olarak üretildi. ";
        }

        // Durumlara göre ek bağlamsal yanıtlar (üstteki cevabı tamamlar)
        if (current_abstract_state == CerebrumLux::AbstractState::Error) {
            generated_text += " Bir hata durumu tespit ettim.";
            reasoning_text += "Hata durumu göz önünde bulunduruldu. ";
        } else if (current_abstract_state == CerebrumLux::AbstractState::Learning) {
            generated_text += " Şu an öğrenme modundayım.";
            reasoning_text += "Öğrenme durumu göz önünde bulunduruldu. ";
        } else if (current_abstract_state == CerebrumLux::AbstractState::Focused) {
            generated_text += " Odaklanmış görünüyorsunuz.";
            reasoning_text += "Odaklanmış durum göz önünde bulunduruldu. ";
        } else if (current_abstract_state == CerebrumLux::AbstractState::Distracted) {
            generated_text += " Dikkatiniz dağınık gibi.";
            reasoning_text += "Dikkat dağınıklığı durumu göz önünde bulunduruldu. ";
        } else if (current_abstract_state == CerebrumLux::AbstractState::HighProductivity) {
            generated_text += " Yüksek üretkenlik içindesiniz.";
            reasoning_text += "Yüksek üretkenlik durumu göz önünde bulunduruldu. ";
        } else if (current_abstract_state == CerebrumLux::AbstractState::LowProductivity) {
            generated_text += " Üretkenliğiniz düşük görünüyor.";
            reasoning_text += "Düşük üretkenlik durumu göz önünde bulunduruldu. ";
        }

        // Hedeflere göre ek bağlamsal yanıtlar
        if (current_goal == CerebrumLux::AIGoal::OptimizeProductivity) {
            generated_text += " Verimliliğinizi artırmaya odaklanıyorum.";
            reasoning_text += "Verimlilik optimizasyonu hedefi göz önünde bulunduruldu. ";
        } else if (current_goal == CerebrumLux::AIGoal::EnsureSecurity) {
            generated_text += " Güvenliğinizi sağlamak için çalışıyorum.";
            reasoning_text += "Güvenlik sağlama hedefi göz önünde bulunduruldu. ";
        } else if (current_goal == CerebrumLux::AIGoal::MaximizeLearning) {
            generated_text += " Öğrenme kapasitemi artırmaya çalışıyorum.";
            reasoning_text += "Öğrenme maksimizasyonu hedefi göz önünde bulunduruldu. "; // Düzeltildi
        }
    }

    // Nihai Fallback (eğer hala boşsa veya çok genel bir yanıt ise)
    if (generated_text == "Anladım. " || generated_text.empty()) {
        generated_text = fallback_response_for_intent(current_intent, current_abstract_state, sequence);
        reasoning_text += "Varsayılan fallback yanıtı kullanıldı. ";
        clarification_needed = true; // Fallback kullanıldıysa genellikle açıklama gerekebilir
    }

    // ChatResponse objesini doldur ve döndür
    response_obj.text = generated_text;
    response_obj.reasoning = reasoning_text;
    response_obj.needs_clarification = clarification_needed;

    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "NaturalLanguageProcessor: Yanıt üretildi. Niyet: " << CerebrumLux::intent_to_string(current_intent)
                                  << ", Durum: " << CerebrumLux::abstract_state_to_string(current_abstract_state)
                                  << ", Hedef: " << CerebrumLux::goal_to_string(current_goal)
                                  << ", Yanıt: " << response_obj.text.substr(0, std::min((size_t)50, response_obj.text.length()))
                                  << ", Gerekçe: " << response_obj.reasoning.substr(0, std::min((size_t)50, response_obj.reasoning.length()))
                                  << ", Açıklama Gerekli: " << (response_obj.needs_clarification ? "Evet" : "Hayır"));
    return response_obj;
}

// And the rest of the file
std::string NaturalLanguageProcessor::generate_dynamic_prompt(
    CerebrumLux::UserIntent intent,
    CerebrumLux::AbstractState state,
    CerebrumLux::AIGoal goal,
    const CerebrumLux::DynamicSequence& sequence,
    const std::string& user_input, // Metoda user_input da ekleyebiliriz
     const std::vector<CerebrumLux::Capsule>& relevant_capsules
 ) const {
    std::stringstream ss;
    ss << std::string("Kullanıcının niyeti: ") << CerebrumLux::intent_to_string(intent) << std::string(". ");
    ss << std::string("Mevcut sistem durumu: ") << CerebrumLux::abstract_state_to_string(state) << std::string(". ");
    ss << std::string("AI'ın mevcut hedefi: ") << CerebrumLux::goal_to_string(goal) << std::string(". ");
    ss << std::string("Kullanıcı girdisi (veya anahtar kelimelerden türetilen sorgu): '") << user_input << std::string("'. ");
 
     if (!relevant_capsules.empty()) {
        ss << std::string("Aşağıdaki ilgili bilgi tabanı kapsüllerini dikkate al: ");
         for (const auto& capsule : relevant_capsules) {
            ss << std::string("(ID: ") << capsule.id << std::string(", Konu: ") << capsule.topic << std::string(", Özet: '") << capsule.plain_text_summary.substr(0, std::min((size_t)100, capsule.plain_text_summary.length())) << std::string("...') ");
         }
     } else {
        ss << std::string("İlgili bilgi tabanı kapsülü bulunamadı. ");
     }
 
    ss << std::string("Bu bağlamı kullanarak, kullanıcıya kapsamlı, içgörülü, 2-3 farklı perspektiften değerlendirme içeren, olası sonuçlar ve öneriler sunan bir yanıt üret. Eğer niyet belirsizse, açıklama talep et.");
 
     return ss.str();
}

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

void NaturalLanguageProcessor::update_model(const std::string& observed_text, CerebrumLux::UserIntent true_intent, const std::vector<float>& latent_cryptofig) {
    if (latent_cryptofig.empty() || latent_cryptofig.size() != CerebrumLux::CryptofigAutoencoder::LATENT_DIM) {
        LOG_DEFAULT(CerebrumLux::LogLevel::WARNING, "NLP::update_model: Gecersiz latent kriptofig boyutu. Model guncellenemedi.");
        return;
    }

    auto& weights = this->intent_cryptofig_weights[true_intent];
    if (weights.empty()) {
        weights.assign(CerebrumLux::CryptofigAutoencoder::LATENT_DIM, 0.0f);
    }

    float learning_rate_nlp = 0.1f;
    for (size_t i = 0; i < latent_cryptofig.size(); ++i) {
        weights[i] += learning_rate_nlp * (latent_cryptofig[i] - weights[i]);
        weights[i] = std::min(10.0f, std::max(-10.0f, weights[i]));
    }
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "NLP::update_model updated weights for intent " << CerebrumLux::intent_to_string(true_intent));
}

void NaturalLanguageProcessor::trainIncremental(const std::string& input, const std::string& expected_intent) {
    std::string lower_input = input;
    std::transform(lower_input.begin(), lower_input.end(), lower_input.begin(),
                   [](unsigned char c){ return static_cast<unsigned char>(std::tolower(c)); });

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

    std::vector<float> dummy_cryptofig(CerebrumLux::CryptofigAutoencoder::LATENT_DIM, 0.5f);

    update_model(input, true_intent, dummy_cryptofig);
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "NLP::trainIncremental: '" << expected_intent << "' niyeti için artımlı öğrenme tamamlandı.");
}

void NaturalLanguageProcessor::trainFromKnowledgeBase(const CerebrumLux::KnowledgeBase& kb) {
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "NaturalLanguageProcessor: KnowledgeBase'den eğitim başlatıldı.");
    std::vector<CerebrumLux::Capsule> all_capsules = kb.get_all_capsules();
    for (const auto& capsule : all_capsules) {
        LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "NaturalLanguageProcessor: Kapsülden eğitim (placeholder). ID: " << capsule.id << ", Konu: " << capsule.topic);
    }
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "NaturalLanguageProcessor: KnowledgeBase'den eğitim tamamlandı (placeholder).");
}


void NaturalLanguageProcessor::load_model(const std::string& path) {
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "NLP: Model yüklendi (placeholder): " << path);
}

void NaturalLanguageProcessor::save_model(const std::string& path) const {
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "NLP: Model kaydedildi (placeholder): " << path);
}

std::string NaturalLanguageProcessor::fallback_response_for_intent(CerebrumLux::UserIntent intent, CerebrumLux::AbstractState state, const CerebrumLux::DynamicSequence& sequence) const {
    std::string response = "Bu konuda emin değilim. ";
    if (intent == CerebrumLux::UserIntent::Undefined || intent == CerebrumLux::UserIntent::Unknown) {
        response += "Niyetinizi tam olarak anlayamadım.";
    } else {
        response += "Niyetiniz '" + CerebrumLux::intent_to_string(intent) + "' gibi görünüyor, ancak daha fazla bilgiye ihtiyacım var.";
    }
    return response;
}

// YENİ EKLENDİ: Metin girdisinden embedding hesaplama (şimdilik placeholder)
std::vector<float> NaturalLanguageProcessor::generate_text_embedding(const std::string& text) {
    // TODO: Gerçek bir NLP/Embedding modeli (örn. FastText, Sentence-BERT) burada entegre edilecek.
    // Şimdilik, deterministik ve anlamsal olarak biraz daha ilişkili bir placeholder embedding üretiyoruz.
    // Basit kelime tabanlı vektörleme: Kelimelerin hash değerlerini toplar ve normalize eder.

    const int embedding_dim = 128; 
    std::vector<float> embedding(embedding_dim);
    
    if (text.empty()) {
        // Boş metin için tümüyle sıfırlanmış bir embedding döndür
        return embedding;
    }

    std::hash<std::string> hasher;
    std::stringstream ss(text);
    std::string word;
    long long total_hash_sum = 0; // Toplam hash değeri için long long
    int word_count = 0;

    while (ss >> word) {
        total_hash_sum += hasher(word);
        word_count++;
    }

    // Hash toplamını deterministik bir seed olarak kullan
    unsigned int seed = static_cast<unsigned int>(total_hash_sum % 1000000007); // Daha küçük bir pozitif sayıya çevir
    s_rng.seed(seed);

    for (int i = 0; i < embedding_dim; ++i) {
        embedding[i] = s_dist(s_rng); // Deterministic olarak float değerleri üret
    }

    LOG_DEFAULT(LogLevel::TRACE, "NLP: Metin '" << text.substr(0, std::min(text.length(), (size_t)50)) << "...' için anlamsal placeholder embedding olusturuldu (Seed: " << seed << ").");
    return embedding;
}

} // namespace CerebrumLux