#ifndef INTENT_ANALYZER_H
#define INTENT_ANALYZER_H

#include <string>
#include <vector>
#include <map>
#include <algorithm> // std::max için
#include "../core/enums.h" // UserIntent, AIAction için
#include "../data_models/dynamic_sequence.h" // DynamicSequence için
#include "intent_template.h" // IntentTemplate için

namespace CerebrumLux { // IntentAnalyzer sınıfı bu namespace içine alınacak

class IntentAnalyzer {
public:
    IntentAnalyzer();

    virtual UserIntent analyze_intent(const DynamicSequence& sequence);
    virtual float get_confidence_for_intent(UserIntent intent_id, const std::vector<float>& features) const;

    // Niyet şablonlarını yönetme
    void add_intent_template(const IntentTemplate& new_template);
    void update_template_weights(UserIntent intent_id, const std::vector<float>& new_weights);
    void update_action_success_score(UserIntent intent_id, AIAction action, float score_change);
    std::vector<float> get_intent_weights(UserIntent intent_id) const;

    // Performans izleme
    float get_last_confidence() const { return last_confidence; }
    void report_learning_performance(UserIntent intent_id, float implicit_feedback_avg, float explicit_feedback_avg);

private:
    std::map<UserIntent, IntentTemplate> intent_templates;
    float last_confidence; // En son analiz edilen niyetin güven seviyesi
    
    // Yardımcı fonksiyonlar
    float calculate_cosine_similarity(const std::vector<float>& vec1, const std::vector<float>& vec2) const;
};

} // namespace CerebrumLux

#endif // INTENT_ANALYZER_H
