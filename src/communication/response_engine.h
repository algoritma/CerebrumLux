#ifndef RESPONSE_ENGINE_H
#define RESPONSE_ENGINE_H

#include <string>
#include <vector>
#include <map>
#include "../core/enums.h" // UserIntent, AbstractState, AIGoal, AIAction için
#include "../data_models/dynamic_sequence.h" // DynamicSequence için
#include "../brain/intent_analyzer.h" // IntentAnalyzer için
#include "../learning/KnowledgeBase.h" // YENİ: KnowledgeBase için
#include "../communication/ai_insights_engine.h" // AIInsightsEngine için
#include "../planning_execution/goal_manager.h" // GoalManager için
#include "../communication/natural_language_processor.h" // NaturalLanguageProcessor için

namespace CerebrumLux { // ResponseEngine ve ResponseTemplate struct'ı bu namespace içine alınacak

// Yanıt şablonu yapısı
struct ResponseTemplate {
    std::vector<std::string> responses; // Muhtemel yanıtların listesi
    float priority; // Yanıtın önceliği

    ResponseTemplate() : priority(0.5f) {}
    ResponseTemplate(const std::vector<std::string>& resps, float prio = 0.5f)
        : responses(resps), priority(prio) {}
};

class ResponseEngine {
public:
    ResponseEngine(CerebrumLux::IntentAnalyzer& analyzer, CerebrumLux::GoalManager& goal_manager,
                   CerebrumLux::AIInsightsEngine& insights_engine, CerebrumLux::NaturalLanguageProcessor* nlp);

    virtual std::string generate_response(CerebrumLux::UserIntent current_intent, CerebrumLux::AbstractState current_abstract_state,
                                          CerebrumLux::AIGoal current_goal, const CerebrumLux::DynamicSequence& sequence, const CerebrumLux::KnowledgeBase& kb) const; // YENİ: KnowledgeBase parametresi eklendi

private:
    CerebrumLux::IntentAnalyzer& intent_analyzer;
    CerebrumLux::GoalManager& goal_manager;
    CerebrumLux::AIInsightsEngine& insights_engine;
    CerebrumLux::NaturalLanguageProcessor* nlp_processor; // NLP pointer'ı

    // Yanıt şablonları
    std::map<CerebrumLux::UserIntent, std::map<CerebrumLux::AbstractState, ResponseTemplate>> response_templates; // Namespace ile güncellendi

    // Yardımcı fonksiyonlar
    std::string select_random_response(const std::vector<std::string>& responses) const;
};

} // namespace CerebrumLux

#endif // RESPONSE_ENGINE_H