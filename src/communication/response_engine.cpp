// src/communication/response_engine.cpp - Değiştirilen kısım

#include "response_engine.h"
#include "natural_language_processor.h" // NaturalLanguageProcessor'ın tam tanımı için
#include "../core/logger.h"
#include "../core/enums.h"
#include "../core/utils.h" // intent_to_string, abstract_state_to_string, goal_to_string için
#include <algorithm> // std::min için
#include <sstream>   // Prompt oluşturmak için

namespace CerebrumLux {

ResponseEngine::ResponseEngine(std::unique_ptr<NaturalLanguageProcessor> nlp_processor_ptr)
    : nlp_processor(std::move(nlp_processor_ptr))
{
    // LLM Modelini Yükle
    // Not: Yol relative verilmiştir, uygulamanın çalıştığı yere göre ../data altında arar.
    std::string modelPath = "../data/llm_models/llama-2-7b-chat.Q4_K_M.gguf"; 
    
    if (!llm_engine.load_model(modelPath)) {
        LOG_ERROR_CERR(LogLevel::WARNING, "ResponseEngine: LLM modeli yüklenemedi: " << modelPath << ". Kural tabanlı moda devam edilecek.");
    } else {
        LOG_DEFAULT(LogLevel::INFO, "ResponseEngine: LLM modeli başarıyla yüklendi ve hazır.");
    }
    // PERFORMANS DÜZELTMESİ: Kurucu metottan ağır LLM yükleme işlemi kaldırıldı.
    // Bu işlem artık MainWindow'dan asenkron olarak tetiklenecek.
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "ResponseEngine: Initialized.");
}

ChatResponse ResponseEngine::generate_response(
    CerebrumLux::UserIntent current_intent,
    CerebrumLux::AbstractState current_abstract_state,
    CerebrumLux::AIGoal current_goal,
    const DynamicSequence& sequence,
    const KnowledgeBase& kb
) const { // <--- BURAYA const EKLENDİ
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "ResponseEngine: Yanıt üretimi isteniyor. Niyet: " << CerebrumLux::intent_to_string(current_intent) << ", Durum: " << CerebrumLux::abstract_state_to_string(current_abstract_state));

    // Kapsülden anahtar kelimeler çıkarma (placeholder olarak sequence'den alındı)
    std::vector<std::string> keywords;
    if (!sequence.current_application_context.empty()) {
        keywords.push_back(sequence.current_application_context);
    }

    if (!sequence.network_protocol.empty()) {
        keywords.push_back(sequence.network_protocol);
    }

    // 1. Temel NLP Yanıtını Al
    CerebrumLux::ChatResponse nlp_generated_response = this->nlp_processor->generate_response_text(
        current_intent,
        current_abstract_state,
        current_goal,
        sequence,
        keywords,
        kb
    );

    // --- LLM ENTEGRASYONU ---
    // Eğer LLM yüklü ise ve NLP bir yanıt ürettiyse, bunu zenginleştir.
    // llm_engine 'mutable' olduğu için const metod içinde kullanılabilir.
    // needs_clarification true ise (yani bilgi bulunamadıysa) LLM'e sormaya gerek yok, fallback dönsün.
    if (llm_engine.is_model_loaded() && !nlp_generated_response.text.empty() && !nlp_generated_response.needs_clarification) {
        
        std::string user_input = "";
        if (!sequence.user_input_history.empty()) {
            user_input = sequence.user_input_history.back();
        }

        // Prompt Hazırlama (Llama-2 Formatı - GÜÇLENDİRİLMİŞ TÜRKÇE)
        std::stringstream prompt_ss;
        prompt_ss << "[INST] <<SYS>>\n"
                  << "Sen Cerebrum Lux adında yardımcı bir yapay zeka asistanısın. "
                  << "Kullanıcının sorusuna SADECE TÜRKÇE dilinde yanıt ver. İngilizce kullanma. "
                  << "Aşağıdaki 'Bilgi Tabanı' verilerini kullanarak doğal, akıcı ve profesyonel bir yanıt oluştur. "
                  << "Bilgi tabanındaki [cite:...] referanslarını yanıtının içinde korumaya çalış.\n"
                  << "<</SYS>>\n\n"
                  << "Bilgi Tabanı:\n" << nlp_generated_response.text << "\n\n"
                  << "Kullanıcı Sorusu:\n" << user_input << "\n\n"
                  << "Türkçe Yanıt:\n" // Modeli zorlamak için ipucu
                  << "[/INST]";

        std::string prompt = prompt_ss.str();
        LOG_DEFAULT(LogLevel::DEBUG, "ResponseEngine: LLM Prompt oluşturuldu (" << prompt.length() << " karakter).");

        // LLM Konfigürasyonu
        LLMGenerationConfig config;
        config.max_tokens = 512;
        config.temperature = 0.3f; // Daha tutarlı yanıtlar için düşük sıcaklık

        std::string llm_output = llm_engine.generate(prompt, config);

        if (!llm_output.empty()) {
            nlp_generated_response.text = llm_output; // Yanıtı LLM çıktısı ile değiştir
            nlp_generated_response.reasoning += " (LLM ile işlendi)";
            LOG_DEFAULT(LogLevel::INFO, "ResponseEngine: LLM yanıtı başarıyla üretildi.");
        } else {
            LOG_ERROR_CERR(LogLevel::WARNING, "ResponseEngine: LLM yanıt üretemedi, kural tabanlı yanıt dönülüyor.");
        }
    }
    // ------------------------

    // LOG_DEFAULT çağrısını ChatResponse'un üyelerini kullanacak şekilde güncelle
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "ResponseEngine: Yanıt üretildi. Niyet: " << CerebrumLux::intent_to_string(current_intent)
                                  << ", Yanıt (kısaltılmış): " << nlp_generated_response.text.substr(0, std::min((size_t)50, nlp_generated_response.text.length()))
                                  << ", Açıklama Gerekli: " << (nlp_generated_response.needs_clarification ? "Evet" : "Hayır"));

    // DEĞİŞTİRİLEN KOD: ChatResponse objesini doğrudan döndür
    return nlp_generated_response;
}


} // namespace CerebrumLux