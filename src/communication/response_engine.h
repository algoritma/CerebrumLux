// src/communication/response_engine.h - Değiştirilen kısım

#ifndef RESPONSE_ENGINE_H
#define RESPONSE_ENGINE_H

#include <string>
#include <vector>
#include <memory> // std::unique_ptr için

#include "../core/enums.h"
#include "../data_models/dynamic_sequence.h"
#include "../learning/KnowledgeBase.h"
#include "natural_language_processor.h" // YENİ EKLENDİ: CerebrumLux::ChatResponse için

namespace CerebrumLux {

class NaturalLanguageProcessor; // Forward declaration

class ResponseEngine {
public:
    ResponseEngine(std::unique_ptr<NaturalLanguageProcessor> nlp_processor);

    // DEĞİŞTİRİLEN KOD: generate_response metodunun dönüş tipi ChatResponse oldu
    virtual ChatResponse generate_response(
        CerebrumLux::UserIntent current_intent,
        CerebrumLux::AbstractState current_abstract_state,
        CerebrumLux::AIGoal current_goal,
        const CerebrumLux::DynamicSequence& sequence,
        const CerebrumLux::KnowledgeBase& kb
    ) const;

private:
    std::unique_ptr<NaturalLanguageProcessor> nlp_processor;
};

} // namespace CerebrumLux

#endif // RESPONSE_ENGINE_H