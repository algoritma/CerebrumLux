#include "natural_language_processor.h"
#include "../core/logger.h"
#include "../core/enums.h"         // LogLevel, UserIntent, AbstractState gibi enum'lar için
#include "../core/utils.h"          // intent_to_string, abstract_state_to_string, goal_to_string, action_to_string için
#include "../brain/autoencoder.h"   // CryptofigAutoencoder için (INPUT_DIM, LATENT_DIM)
#include "../learning/KnowledgeBase.h" // KnowledgeBase için
#include "../data_models/dynamic_sequence.h" // DynamicSequence için

#include <iostream>
#include <algorithm> // std::transform için
#include <cctype>    // std::tolower için
#include <sstream>   // stringstream için
#include <functional> // std::hash için (statik embedding için)
#include <algorithm> // std::min için
#include <QtConcurrent/QtConcurrent> // EKLENDİ (Asenkron işlemler için)
#include "../brain/llm_engine.h" // EKLENDİ: Llama-2 Motoruna Erişim
#include "../gui/DataTypes.h" // ChatResponse için

#ifdef _WIN32
#include <Windows.h>
#endif

namespace CerebrumLux {

NaturalLanguageProcessor::NaturalLanguageProcessor(GoalManager& goal_manager_ref, KnowledgeBase& kbRef, QObject* parent)
    : QObject(parent), goal_manager(goal_manager_ref), kbRef_(kbRef) // explicit constructor in header
{
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "NaturalLanguageProcessor: Initialized.");
}

// LLMProcessor'ın kurucusu
LLMProcessor::LLMProcessor(CerebrumLux::GoalManager& goal_manager_ref, CerebrumLux::KnowledgeBase& kbRef, QObject* parent)
    : NaturalLanguageProcessor(goal_manager_ref, kbRef, parent)
{
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "LLMProcessor: Initialized.");
}


ChatResponse LLMProcessor::generate_response_text(
    CerebrumLux::UserIntent current_intent,
    CerebrumLux::AbstractState current_abstract_state,
    CerebrumLux::AIGoal current_goal,
    const CerebrumLux::DynamicSequence& sequence,
    const std::vector<std::string>& relevant_keywords,
    const CerebrumLux::KnowledgeBase& kb,
    const std::vector<float>& user_embedding
) const {
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "NaturalLanguageProcessor: generate_response_text çağrıldı. Niyet: " << intent_to_string(current_intent) << ", Durum: " << abstract_state_to_string(current_abstract_state));

    ChatResponse response_obj;
    std::string generated_text = "";
    std::string reasoning_text = "";
    bool clarification_needed = false;
 
    // 1. Arama Sorgusunu Belirle
    std::string search_term_for_embedding = "";
    
    // ÖNCELİK 1: Sequence geçmişindeki son tam kullanıcı mesajını al (En doğru bağlam)
    if (!sequence.user_input_history.empty()) {
        search_term_for_embedding = sequence.user_input_history.back();
        LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "NLP: Arama sorgusu Sequence tarihçesinden alındı: " << search_term_for_embedding);
    }
    
    // ÖNCELİK 2: Eğer tarihçe boşsa, anahtar kelimeleri veya niyeti kullan
    if (search_term_for_embedding.empty()) {
         if (!relevant_keywords.empty()) {
             search_term_for_embedding = relevant_keywords[0];
             if (relevant_keywords.size() > 1) search_term_for_embedding += " " + relevant_keywords[1];
         } else {
             search_term_for_embedding = intent_to_string(current_intent);
             if (current_intent == CerebrumLux::UserIntent::Question) search_term_for_embedding += " nedir";
         }
    }
 
    // 2. Embedding Oluştur
    std::vector<float> search_query_embedding = user_embedding; // Artık parametre olarak geliyor
 
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "NaturalLanguageProcessor: Semantic arama için oluşturulan embedding (ilk 5 float): " 
                << search_query_embedding[0] << ", " << search_query_embedding[1] << ", " << search_query_embedding[2] << ", " << search_query_embedding[3] << ", " << search_query_embedding[4] 
                << " (Sorgu: '" << search_term_for_embedding << "')");
 
    // 3. Arama Stratejisi
    // Öncelikle, temel tanımlayıcı kapsülleri doğrudan ID ile arayalım.
    std::vector<CerebrumLux::Capsule> definitive_results;
    bool found_by_id = false;

    // "Cerebrum Lux nedir?" benzeri bir soru için
    if (search_term_for_embedding.find("Cerebrum Lux") != std::string::npos || search_term_for_embedding.find("nedir") != std::string::npos || search_term_for_embedding.find("tanım") != std::string::npos) {
        std::optional<CerebrumLux::Capsule> def_capsule = kb.find_capsule_by_id("CerebrumLux_Definition_17000000000000000");
        if (def_capsule) {
            definitive_results.push_back(*def_capsule);
            LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "NaturalLanguageProcessor: Doğrudan ID ile 'Cerebrum Lux Tanımı' kapsülü bulundu.");
            found_by_id = true;
        }
    }
    // "Yeteneklerin neler?" veya "Nasıl yardımcı olabilirsin?" benzeri bir soru için
    if (search_term_for_embedding.find("yetenek") != std::string::npos || search_term_for_embedding.find("nasıl yardımcı olabilirsin") != std::string::npos || current_intent == CerebrumLux::UserIntent::InquireCapability) {
        std::optional<CerebrumLux::Capsule> cap_capsule = kb.find_capsule_by_id("CerebrumLux_Capabilities_17000000000000000");
        if (cap_capsule) {
            definitive_results.push_back(*cap_capsule);
            LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "NaturalLanguageProcessor: Doğrudan ID ile 'Yetenekleri' kapsülü bulundu.");
            found_by_id = true;
        }
    }

    // 4. Sonuçları Hazırla
    std::vector<CerebrumLux::Capsule> final_results_for_synthesis;

    if (found_by_id && !definitive_results.empty()) {
        final_results_for_synthesis = definitive_results;
        LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "NaturalLanguageProcessor: Doğrudan tanımlayıcı kapsüller önceliklendirildi.");
    } else {
        // Doğrudan ID ile bulunamazsa, semantik arama yap (Yanıt + Öneriler için daha fazla sonuç al)
        std::vector<CerebrumLux::Capsule> semantic_search_results = kb.semantic_search(search_query_embedding, 6);
        LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "NaturalLanguageProcessor: KnowledgeBase'den semantik arama yapıldı. Bulunan kapsül sayısı: " << semantic_search_results.size() << " (Sorgu: '" << search_term_for_embedding << "')");
 
        // Semantic arama sonuçlarını da final_results'a ekle, ancak boş veya anlamsız olanları filtrele
        for (const auto& capsule : semantic_search_results) {
            // Kapsül içeriğinin anlamsız olup olmadığını kontrol et
            if (capsule.content.empty() ||
                capsule.content.find("Is this data relevant to AI Insight?") != std::string::npos ||
                capsule.content.find("Bilgi bulunamadı.") != std::string::npos ||
                capsule.plain_text_summary.empty() ||
                capsule.plain_text_summary.find("Is this data relevant to AI Insight?") != std::string::npos ||
                capsule.plain_text_summary.find("Bilgi bulunamadı.") != std::string::npos) {
                LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "NaturalLanguageProcessor: Anlamsız veya boş içeriğe sahip kapsül filtrelendi. ID: " << capsule.id);
                continue;
            }
            final_results_for_synthesis.push_back(capsule);
        }
        LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "NaturalLanguageProcessor: Semantik arama sonuçları filtrelendi ve yanıt sentezi için hazırlandı. Sayı: " << final_results_for_synthesis.size());
    }

    // 5. Yanıt Sentezi (Gelişmiş)
    if (!final_results_for_synthesis.empty()) {
        std::stringstream response_stream;
        std::stringstream reasoning_stream;
        std::vector<std::string> cited_capsule_ids;

        response_stream << "Bilgi tabanımda sorunuzla ilgili bazı bilgiler buldum: \n";
        reasoning_stream << "Yanıt, KnowledgeBase'deki ";

        size_t used_capsule_count = 0;
        // En alakalı kapsülleri birleştirerek yanıt oluştur
        for (size_t i = 0; i < final_results_for_synthesis.size(); ++i) {
            const CerebrumLux::Capsule& capsule = final_results_for_synthesis[i];
            cited_capsule_ids.push_back(capsule.id);
            
            // YENİ LOG: Bulunan her kapsülün ID'sini, konusunu ve içeriğini (özet veya tam) logla.
            LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "NaturalLanguageProcessor: Bulunan Kapsül - ID: " << capsule.id 
                        << ", Konu: " << capsule.topic 
                        << ", Özet: '" << capsule.plain_text_summary.substr(0, std::min((size_t)100, capsule.plain_text_summary.length())) << "'"
                        << ", İçerik Boyutu: " << capsule.content.length()
                        << ", İçerik (İlk 100 char): '" << capsule.content.substr(0, std::min((size_t)100, capsule.content.length())) << "...'");

            // Kapsülün içeriğini veya özetini kullanarak daha zengin bir yanıt oluştur
            std::string display_content = capsule.content;
            if (display_content.empty() ||
                display_content.find("Is this data relevant to AI Insight?") != std::string::npos ||
                display_content.find("Bilgi bulunamadı.") != std::string::npos)
            {
                display_content = capsule.plain_text_summary; // İçerik boşsa özeti kullan
                if (display_content.empty() ||
                    display_content.find("Is this data relevant to AI Insight?") != std::string::npos ||
                    display_content.find("Bilgi bulunamadı.") != std::string::npos)
                {
                    display_content = "İlgili bilgi mevcut değil."; // Hem içerik hem özet boşsa veya anlamsızsa
                }
            }
            
            // Metni biraz kırp (çok uzunsa) ve sonuna ... ekle
            std::string content_snippet = display_content.substr(0, std::min((size_t)400, display_content.length()));
            if (display_content.length() > 400) content_snippet += "...";

            response_stream << "\n- **" << capsule.topic << "**: " << content_snippet << " [cite:" << capsule.id << "]";

            reasoning_stream << "'" << capsule.topic << "' (ID: " << capsule.id << ")";
            if (i < final_results_for_synthesis.size() - 1) {
                reasoning_stream << ", ";
            }
            used_capsule_count++;
            
            // İlk 2 kapsülü yanıt için kullan, gerisini önerilere sakla
            if (used_capsule_count >= 2) break;
        }
        reasoning_stream << " kapsüllerinin sentezlenmesiyle oluşturuldu.";

        generated_text = response_stream.str();
        reasoning_text = reasoning_stream.str();

        if (used_capsule_count > 1) {
            generated_text += "\nDaha detaylı bilgi için yukarıdaki referanslara tıklayabilirsiniz.";
        }

        // YENİ: Geriye kalan kapsüllerden öneri soruları oluştur
        for (size_t i = used_capsule_count; i < final_results_for_synthesis.size(); ++i) {
            const CerebrumLux::Capsule& cap = final_results_for_synthesis[i];
            if (!cap.topic.empty() && cap.topic != "AI Insight") { // "AI Insight" başlığı çok genel, onu atla
                 response_obj.suggested_questions.push_back(cap.topic + " hakkında bilgi ver");
            }
        }
        // Eğer hiç öneri çıkmazsa genel bir tane ekle
        if (response_obj.suggested_questions.empty()) {
            response_obj.suggested_questions.push_back("Yeteneklerin neler?");
        }
        
        LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "NaturalLanguageProcessor: KnowledgeBase'den sentezlenmiş yanıt üretildi. Yanıt uzunluğu: " << generated_text.length());
    } else {
        LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "NaturalLanguageProcessor: KnowledgeBase'de ilgili kapsül bulunamadı. Kural tabanlı yanıta dönülüyor.");
        
        // Adım 2: Kural tabanlı yanıt üretimi ve dinamik prompt kullanımı
        std::string dynamic_prompt = generate_dynamic_prompt(current_intent, current_abstract_state, current_goal, sequence, search_term_for_embedding, final_results_for_synthesis); 
        LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "NaturalLanguageProcessor: Dinamik Olarak Üretilen Prompt: " << dynamic_prompt);
    }

    // Durum ve hedefe dayalı bağlamsal eklemeler yap
    std::string contextual_additions = "";
    std::string contextual_reasoning = "";
    
    if (current_goal == CerebrumLux::AIGoal::OptimizeProductivity) { contextual_additions += " \nVerimliliğinizi artırmaya odaklanıyorum."; }
    
    // Eğer KnowledgeBase'den yanıt alınamadıysa ve generated_text hala boşsa, fallback yanıtı kullan
    if (generated_text.empty() || generated_text == "Bilgi tabanımda sorunuzla ilgili bazı bilgiler buldum: \n") {
        generated_text = fallback_response_for_intent(current_intent, current_abstract_state, sequence);
        reasoning_text = "KnowledgeBase'de doğrudan ilgili bilgi bulunamadığı için kural tabanlı fallback yanıt kullanıldı. " + contextual_reasoning;
        clarification_needed = true;
    }

    generated_text += contextual_additions;

    // ChatResponse objesini doldur ve döndür
    response_obj.text = generated_text;
    response_obj.reasoning = reasoning_text;
    response_obj.needs_clarification = clarification_needed;

    return response_obj;
}

std::vector<float> LLMProcessor::generate_text_embedding_sync(const std::string& text, CerebrumLux::Language lang) const {
    // STRATEJİ: Dinamik Zeka (Adaptive Compute)
    // 1. ÖNCELİK: Llama-2 (Unified Brain).
    // Zaten RAM'de olan "Işık Beyin"i kullanıyoruz. Bu hem daha zeki hem de ekstra RAM harcamaz.
    if (LLMEngine::global_instance && LLMEngine::global_instance->is_model_loaded()) {
        std::vector<float> original_embedding = LLMEngine::global_instance->get_embedding(text);
        if (!original_embedding.empty()) {
            LOG_DEFAULT(LogLevel::DEBUG, "NLP: Llama-2 motorundan " + std::to_string(original_embedding.size()) + " boyutlu embedding üretildi.");
            // Boyut düşürme işlemini LLMEngine'deki statik metot ile yap
            return LLMEngine::reduce_embedding_dimension(original_embedding, CerebrumLux::CryptofigAutoencoder::INPUT_DIM);
        }
    }

    const int embedding_dim = CerebrumLux::CryptofigAutoencoder::INPUT_DIM;
    std::vector<float> embedding(embedding_dim, 0.0f);
    if (text.empty()) return embedding;
    std::hash<std::string> hasher;
    size_t hash_val = hasher(text);
    for (int i = 0; i < embedding_dim; ++i) embedding[i] = static_cast<float>(std::sin(static_cast<double>(hash_val) + i));
    LOG_DEFAULT(LogLevel::WARNING, "NLP: Llama-2 motoru kullanılamadığı için fallback (hash tabanlı) embedding üretildi.");
    return embedding;
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

CerebrumLux::UserIntent NaturalLanguageProcessor::infer_intent_from_text(const std::string& text) const {
    // Bu fonksiyonun gerçek bir implementasyonu gereklidir.
    // Şimdilik kural tabanlı bir tahmin yapalım.
    std::string lower_text = text;
    std::transform(lower_text.begin(), lower_text.end(), lower_text.begin(),
                   [](unsigned char c){ return static_cast<unsigned char>(std::tolower(c)); });
    return rule_based_intent_guess(lower_text);
}

// YENİ: Asenkron embedding isteğini işleyen fonksiyon
void NaturalLanguageProcessor::request_embedding_async(const std::string& text, const std::string& request_id) {
    // QtConcurrent::run ile senkron embedding fonksiyonunu arka planda çalıştır
    QtConcurrent::run([this, text, request_id]() {
        // Arka plan thread'inde senkron metodu çağır
        std::vector<float> embedding = this->generate_text_embedding_sync(text);
        // Sonuç hazır olduğunda sinyali yay
        emit this->embeddingReady(request_id, embedding);
    });
}

// YENİ: TeacherInvoker gibi iç sistemler için basit yanıt üretme metodu
std::string NaturalLanguageProcessor::generate_simple_response(const std::string& prompt) const {
    LOG_DEFAULT(CerebrumLux::LogLevel::WARNING, "NaturalLanguageProcessor::generate_simple_response çağrıldı. Bu temel sınıf ve bir şey yapmamalı.");
    return "Base implementation of generate_simple_response. This should be overridden.";
}

// YENİ: TeacherInvoker gibi iç sistemler için basit yanıt üretme metodu
std::string LLMProcessor::generate_simple_response(const std::string& prompt) const {
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "LLMProcessor: generate_simple_response çağrıldı.");
    if (LLMEngine::global_instance && LLMEngine::global_instance->is_model_loaded()) {
        // LLMEngine'in senkron generate metodunu çağırıyoruz.
        return LLMEngine::global_instance->generate(prompt);
    }
    LOG_DEFAULT(CerebrumLux::LogLevel::WARNING, "LLMProcessor: Llama motoru mevcut değil. Basit yanıt için fallback kullanılıyor.");
    return "Fallback response: LLaMA engine not available.";
}


} // namespace CerebrumLux