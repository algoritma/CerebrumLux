#include "response_engine.h"
#include "../core/logger.h"
#include "../core/utils.h" // intent_to_string, abstract_state_to_string, goal_to_string için
#include "../core/enums.h" // UserIntent, AbstractState, AIGoal, AIAction için
#include <random> // rastgele yanıt seçimi için
#include <algorithm> // std::max, std::min için

namespace CerebrumLux { // TÜM İMPLEMENTASYON BU NAMESPACE İÇİNDE OLACAK

ResponseEngine::ResponseEngine(CerebrumLux::IntentAnalyzer& analyzer_ref, CerebrumLux::GoalManager& goal_manager_ref,
                               CerebrumLux::AIInsightsEngine& insights_engine_ref, CerebrumLux::NaturalLanguageProcessor* nlp_ref)
    : intent_analyzer(analyzer_ref),
      goal_manager(goal_manager_ref),
      insights_engine(insights_engine_ref),
      nlp_processor(nlp_ref)
{
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "ResponseEngine: Initialized.");

    // Yanıt şablonlarını başlat
    // UserIntent::FastTyping için
    this->response_templates[CerebrumLux::UserIntent::FastTyping][CerebrumLux::AbstractState::HighProductivity].responses = {
        "Çok hızlı yazıyorsunuz! Harika bir üretkenlik. ",
        "Yazma hızınız etkileyici! Üretkenliğiniz zirvede. "
    };
    this->response_templates[CerebrumLux::UserIntent::FastTyping][CerebrumLux::AbstractState::NormalOperation].responses = {
        "Hızlı yazım algılandı. ",
        "Yazım hızınız iyi. "
    };

    // UserIntent::Editing için
    this->response_templates[CerebrumLux::UserIntent::Editing][CerebrumLux::AbstractState::Focused].responses = {
        "Odaklanmış bir düzenleme yapıyorsunuz. ",
        "Düzenleme sürecinde tam konsantrasyon. "
    };

    // Varsayılan boş durum için
    this->response_templates[CerebrumLux::UserIntent::Undefined][CerebrumLux::AbstractState::Idle].responses = {
        "Boşta ve bekliyorum. ",
        "Sizden bir girdi bekliyorum. "
    };
    
    // UserIntent::Unknown için (eğer enums.h'ye Unknown eklendiyse)
    this->response_templates[CerebrumLux::UserIntent::Unknown][CerebrumLux::AbstractState::Undefined].responses = { 
        "Niyetinizi tam anlayamadım. ",
        "Lütfen daha açık ifade edin. "
    };
    this->response_templates[CerebrumLux::UserIntent::Unknown][CerebrumLux::AbstractState::LowProductivity].responses = { 
        "Niyetinizi anlayamıyorum ve verimliliğiniz düşük. "
    };
    this->response_templates[CerebrumLux::UserIntent::Unknown][CerebrumLux::AbstractState::Distracted].responses = { 
        "Niyetinizi anlayamıyorum ve dikkat dağınıklığı algılıyorum. "
    };

    // Hata veya donanım sorunu durumları
    this->response_templates[CerebrumLux::UserIntent::Undefined][CerebrumLux::AbstractState::FaultyHardware].responses = {
        "Donanımınızda bir sorun algıladım. Kontrol etmenizi öneririm. ",
        "Sisteminizde donanım hatası olabilir. "
    };

    // UserIntent::Programming için
    this->response_templates[CerebrumLux::UserIntent::Programming][CerebrumLux::AbstractState::Focused].responses = {
        "Kodlama sürecinizde odaklanmış görünüyorsunuz. İyi çalışmalar! ",
        "Programlama yaparken sizi gözlemliyorum, işinizde iyisiniz. "
    };
    this->response_templates[CerebrumLux::UserIntent::Programming][CerebrumLux::AbstractState::NormalOperation].responses = {
        "Programlama aktivitesi algılandı. "
    };
    this->response_templates[CerebrumLux::UserIntent::Programming][CerebrumLux::AbstractState::Debugging].responses = {
        "Hata ayıklama sürecinde misiniz? Belki yardımcı olabilirim. ",
        "Debug yaparken karşılaştığınız bir sorun var mı? "
    };

    // UserIntent::Gaming için
    this->response_templates[CerebrumLux::UserIntent::Gaming][CerebrumLux::AbstractState::Focused].responses = {
        "Oyun keyfinizi bölmek istemem, tam odaklanmış görünüyorsunuz. ",
        "Gaming modundasınız, iyi eğlenceler! "
    };
    this->response_templates[CerebrumLux::UserIntent::Gaming][CerebrumLux::AbstractState::Distracted].responses = {
        "Oyun oynarken dikkatiniz dağınık mı? Yardımcı olabilirim. ",
        "Gaming modundasınız ancak odaklanma sorunu yaşıyor gibisiniz. "
    };

    // UserIntent::MediaConsumption için
    this->response_templates[CerebrumLux::UserIntent::MediaConsumption][CerebrumLux::AbstractState::PassiveConsumption].responses = {
        "Medya tüketimi yapıyorsunuz, rahatlayın. ",
        "Bir şeyler izliyor veya dinliyorsunuz. "
    };
    this->response_templates[CerebrumLux::UserIntent::MediaConsumption][CerebrumLux::AbstractState::NormalOperation].responses = {
        "Medya tüketim aktivitesi algılandı. "
    };

    // UserIntent::CreativeWork için
    this->response_templates[CerebrumLux::UserIntent::CreativeWork][CerebrumLux::AbstractState::CreativeFlow].responses = {
        "Yaratıcı akış durumundasınız. İlhamınız bol olsun! ",
        "Harika bir yaratıcı çalışma yapıyorsunuz. "
    };
    this->response_templates[CerebrumLux::UserIntent::CreativeWork][CerebrumLux::AbstractState::NormalOperation].responses = {
        "Yaratıcı çalışma aktivitesi algılandı. "
    };

    // UserIntent::Research için
    this->response_templates[CerebrumLux::UserIntent::Research][CerebrumLux::AbstractState::SeekingInformation].responses = {
        "Araştırma yapıyor ve bilgi arıyorsunuz. ",
        "Bilgi edinme sürecinde size destek olabilirim. "
    };

    // UserIntent::Communication için
    this->response_templates[CerebrumLux::UserIntent::Communication][CerebrumLux::AbstractState::SocialInteraction].responses = {
        "Sosyal etkileşim halindesiniz. ",
        "İletişim kurarken size eşlik ediyorum. "
    };
    this->response_templates[CerebrumLux::UserIntent::Communication][CerebrumLux::AbstractState::NormalOperation].responses = {
        "İletişim aktivitesi algılandı. "
    };


    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "ResponseEngine: Response templates initialized.");
}


// Yanıt üretimi
std::string ResponseEngine::generate_response(
    CerebrumLux::UserIntent current_intent,
    CerebrumLux::AbstractState current_abstract_state,
    CerebrumLux::AIGoal current_goal,
    const CerebrumLux::DynamicSequence& sequence,
    const CerebrumLux::KnowledgeBase& kb // YENİ: KnowledgeBase parametresi kullanıldı
) const { 
    
    LOG_DEFAULT(LogLevel::DEBUG, "ResponseEngine: Yanıt üretiliyor. Niyet: " << intent_to_string(current_intent) << ", Durum: " << abstract_state_to_string(current_abstract_state) << ", Hedef: " << goal_to_string(current_goal));

    std::string response_text;

    // 1. Niyet ve Duruma Özel Yanıtlar (kural tabanlı, KnowledgeBase'den sonra)
    auto intent_it = this->response_templates.find(current_intent);
    if (intent_it != this->response_templates.end()) {
        auto state_it = intent_it->second.find(current_abstract_state);
        if (state_it != intent_it->second.end()) {
            response_text = select_random_response(state_it->second.responses);
        }
    }

    // 2. NLP İşlemcisinden Ek Bağlamsal Yanıt (KnowledgeBase'i kullanarak)
    if (this->nlp_processor) {
        std::vector<std::string> keywords; // Dinamik olarak anahtar kelimeler oluşturulabilir. Örnek:
        // keywords.push_back(sequence.current_application_context); // Uygulama bağlamı anahtar kelime olabilir
        // ... (diğer anahtar kelimeler) ...

        // NLP Processor'ı çağırırken KnowledgeBase'i iletiyoruz
        std::string nlp_generated_response = this->nlp_processor->generate_response_text(current_intent, current_abstract_state, current_goal, sequence, keywords, kb);
        if (!nlp_generated_response.empty() && nlp_generated_response != "Anladım. Niyetinizi tam olarak anlayamadım.") { // NLP'den boş veya genel bir yanıt gelmediyse
            response_text += nlp_generated_response; // Mevcut yanıta ekle
        }
    }

    // 3. Eğer hala boşsa veya yetersizse, hedefe dayalı genel bir yanıt
    if (response_text.empty() || response_text == "Anladım. ") {
        if (current_goal == CerebrumLux::AIGoal::OptimizeProductivity) {
            response_text = "Verimliliğinizi optimize etmek için elimden geleni yapıyorum. ";
        } else if (current_goal == CerebrumLux::AIGoal::EnsureSecurity) {
            response_text = "Sisteminizin güvenliğini sağlıyorum. ";
        }
    }

    // 4. Nihai Fallback (nlp_processor üzerinden çağrıldı)
    if (response_text.empty() || response_text == "Anladım. ") {
        response_text = nlp_processor->fallback_response_for_intent(current_intent, current_abstract_state, sequence); // DÜZELTİLDİ
    }

    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "ResponseEngine: Yanıt üretildi. Niyet: " << CerebrumLux::intent_to_string(current_intent)
                                  << ", Durum: " << CerebrumLux::abstract_state_to_string(current_abstract_state)
                                  << ", Hedef: " << CerebrumLux::goal_to_string(current_goal)
                                  << ", Yanıt: " << response_text.substr(0, std::min((size_t)50, response_text.length())));
    return response_text;
}

// Rastgele yanıt seçimi
std::string ResponseEngine::select_random_response(const std::vector<std::string>& responses) const {
    if (responses.empty()) {
        return "";
    }
    std::uniform_int_distribution<size_t> dist(0, responses.size() - 1);
    return responses[dist(CerebrumLux::SafeRNG::get_instance().get_generator())];
}

} // namespace CerebrumLux