// src/communication/response_engine.h - Değiştirilen kısım

#ifndef RESPONSE_ENGINE_H
#define RESPONSE_ENGINE_H

#include <string>
#include <vector>
#include <memory> // std::unique_ptr için

#include "../core/enums.h"
#include "../data_models/dynamic_sequence.h"
#include "../learning/KnowledgeBase.h"
#include "../brain/llm_engine.h" 
#include "../gui/DataTypes.h" // EKLENDİ: ChatResponse için (Artık buradan geliyor)
#include "natural_language_processor.h" // NaturalLanguageProcessor'ın tam tanımı için

namespace CerebrumLux {

class NaturalLanguageProcessor; // Forward declaration

class ResponseEngine {
public:
    ResponseEngine(std::unique_ptr<NaturalLanguageProcessor> nlp_processor);
    ResponseEngine(std::unique_ptr<NaturalLanguageProcessor> nlp_processor, LLMEngine llm_engine); // YENİ: LLM Motorunu dışarıdan almak için
    // DEĞİŞTİRİLEN KOD: generate_response metodunun dönüş tipi ChatResponse oldu
    virtual ChatResponse generate_response(
        CerebrumLux::UserIntent current_intent,
        CerebrumLux::AbstractState current_abstract_state,
        CerebrumLux::AIGoal current_goal,
        const CerebrumLux::DynamicSequence& sequence,
        const CerebrumLux::KnowledgeBase& kb,
        const std::vector<float>& user_embedding // YENİ: Embedding parametresi
    ) const;

    // YENİ: LLM Motoruna erişim (Gerekirse dışarıdan yapılandırmak için)
    LLMEngine& get_llm_engine() { return llm_engine; }

    // YENİ: LLM modelini asenkron olarak yüklemek için metot
    void load_llm_model_async();

private:
    std::unique_ptr<NaturalLanguageProcessor> nlp_processor;

    mutable LLMEngine llm_engine; // YENİ: LLM Motoru (Const metodlarda çalışması için mutable)

};

} // namespace CerebrumLux

#endif // RESPONSE_ENGINE_H