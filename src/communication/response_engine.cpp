// src/communication/response_engine.cpp - Değiştirilen kısım

#include "response_engine.h"
#include "natural_language_processor.h" // NaturalLanguageProcessor'ın tam tanımı için
#include "../core/logger.h"
#include "../core/enums.h"
#include "../core/utils.h" // intent_to_string, abstract_state_to_string, goal_to_string için
#include <algorithm> // std::min için

namespace CerebrumLux {

ResponseEngine::ResponseEngine(std::unique_ptr<NaturalLanguageProcessor> nlp_processor_ptr)
    : nlp_processor(std::move(nlp_processor_ptr))
{
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "ResponseEngine: Initialized.");
}

// DEĞİŞTİRİLEN KOD: generate_response metodunun implementasyon dönüş tipi ChatResponse oldu
ChatResponse ResponseEngine::generate_response(
    CerebrumLux::UserIntent current_intent,
    CerebrumLux::AbstractState current_abstract_state,
    CerebrumLux::AIGoal current_goal,
    const CerebrumLux::DynamicSequence& sequence,
    const CerebrumLux::KnowledgeBase& kb
) const {
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "ResponseEngine: Yanıt üretimi isteniyor. Niyet: " << CerebrumLux::intent_to_string(current_intent) << ", Durum: " << CerebrumLux::abstract_state_to_string(current_abstract_state));

    // Kapsülden anahtar kelimeler çıkarma (placeholder olarak sequence'den alındı)
    std::vector<std::string> keywords;
    // Basit bir örnek: DynamicSequence'deki bağlam bilgilerini anahtar kelime olarak kullan
    if (!sequence.current_application_context.empty()) {
        keywords.push_back(sequence.current_application_context);
    }
    if (!sequence.network_protocol.empty()) {
        keywords.push_back(sequence.network_protocol);
    }
    // Daha gerçekçi bir senaryoda, NLP bu keywords'leri kullanıcı girdisinden veya bağlamdan çıkarır.

    // DEĞİŞTİRİLEN KOD: nlp_processor'dan gelen yanıtı ChatResponse olarak al
    CerebrumLux::ChatResponse nlp_generated_response = this->nlp_processor->generate_response_text(
        current_intent,
        current_abstract_state,
        current_goal,
        sequence,
        keywords,
        kb
    );

    // LOG_DEFAULT çağrısını ChatResponse'un üyelerini kullanacak şekilde güncelle
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "ResponseEngine: Yanıt üretildi. Niyet: " << CerebrumLux::intent_to_string(current_intent)
                                  << ", Durum: " << CerebrumLux::abstract_state_to_string(current_abstract_state)
                                  << ", Hedef: " << CerebrumLux::goal_to_string(current_goal)
                                  << ", Yanıt (kısaltılmış): " << nlp_generated_response.text.substr(0, std::min((size_t)50, nlp_generated_response.text.length()))
                                  << ", Gerekçe (kısaltılmış): " << nlp_generated_response.reasoning.substr(0, std::min((size_t)50, nlp_generated_response.reasoning.length()))
                                  << ", Açıklama Gerekli: " << (nlp_generated_response.needs_clarification ? "Evet" : "Hayır"));

    // DEĞİŞTİRİLEN KOD: ChatResponse objesini doğrudan döndür
    return nlp_generated_response;
}

} // namespace CerebrumLux