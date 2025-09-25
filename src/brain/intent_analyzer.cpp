#include "intent_analyzer.h"
#include "../core/logger.h"
#include "../core/utils.h" // intent_to_string, action_to_string için
#include <numeric> // std::accumulate için
#include <cmath>   // std::sqrt için
#include "../brain/autoencoder.h" // CryptofigAutoencoder::INPUT_DIM için

namespace CerebrumLux {

IntentAnalyzer::IntentAnalyzer()
    : last_confidence(0.0f)
{
    // Varsayılan niyet şablonlarını başlat
    add_intent_template(IntentTemplate(UserIntent::Undefined, std::vector<float>(CryptofigAutoencoder::INPUT_DIM, 0.1f)));
    add_intent_template(IntentTemplate(UserIntent::Question, std::vector<float>(CryptofigAutoencoder::INPUT_DIM, 0.2f)));
    add_intent_template(IntentTemplate(UserIntent::Command, std::vector<float>(CryptofigAutoencoder::INPUT_DIM, 0.3f)));
    add_intent_template(IntentTemplate(UserIntent::Statement, std::vector<float>(CryptofigAutoencoder::INPUT_DIM, 0.4f)));
    add_intent_template(IntentTemplate(UserIntent::FastTyping, std::vector<float>(CryptofigAutoencoder::INPUT_DIM, 0.5f)));

    LOG_DEFAULT(LogLevel::INFO, "IntentAnalyzer: Initialized with default intent templates.");
}

UserIntent IntentAnalyzer::analyze_intent(const DynamicSequence& sequence) {
    if (sequence.statistical_features_vector.empty()) {
        LOG_DEFAULT(LogLevel::WARNING, "IntentAnalyzer::analyze_intent: Bos statistical_features_vector, niyet analiz edilemiyor.");
        last_confidence = 0.0f;
        return UserIntent::Undefined;
    }

    UserIntent best_intent = UserIntent::Undefined;
    float highest_confidence = 0.0f;

    for (const auto& pair : intent_templates) {
        UserIntent current_intent_id = pair.first;
        const IntentTemplate& current_template = pair.second;

        float confidence = get_confidence_for_intent(current_intent_id, sequence.statistical_features_vector);

        if (confidence > highest_confidence) {
            highest_confidence = confidence;
            best_intent = current_intent_id;
        }
    }

    last_confidence = highest_confidence;
    LOG_DEFAULT(LogLevel::DEBUG, "IntentAnalyzer::analyze_intent: Analiz edilen niyet: " << CerebrumLux::intent_to_string(best_intent) << ", Güven: " << highest_confidence); // GÜNCELLENDİ
    return best_intent;
}

float IntentAnalyzer::get_confidence_for_intent(UserIntent intent_id, const std::vector<float>& features) const {
    auto it = intent_templates.find(intent_id);
    if (it == intent_templates.end()) {
        return 0.0f;
    }

    const IntentTemplate& t = it->second;
    return calculate_cosine_similarity(features, t.weights);
}

void IntentAnalyzer::add_intent_template(const IntentTemplate& new_template) {
    auto [it, inserted] = intent_templates.insert({new_template.id, new_template});
    if (inserted) {
        LOG_DEFAULT(LogLevel::INFO, "IntentAnalyzer: Yeni niyet şablonu eklendi: " << CerebrumLux::intent_to_string(new_template.id)); // GÜNCELLENDİ
    } else {
        LOG_DEFAULT(LogLevel::WARNING, "IntentAnalyzer: Niyet şablonu zaten mevcut: " << CerebrumLux::intent_to_string(new_template.id) << ". Güncelleniyor."); // GÜNCELLENDİ
        it->second = new_template;
    }
}

void IntentAnalyzer::update_template_weights(UserIntent intent_id, const std::vector<float>& new_weights) {
    auto it = intent_templates.find(intent_id);
    if (it != intent_templates.end()) {
        it->second.weights = new_weights;
        LOG_DEFAULT(LogLevel::DEBUG, "IntentAnalyzer: Niyet şablonu ağırlıkları güncellendi: " << CerebrumLux::intent_to_string(intent_id)); // GÜNCELLENDİ
    } else {
        LOG_DEFAULT(LogLevel::WARNING, "IntentAnalyzer: Niyet şablonu bulunamadı: " << CerebrumLux::intent_to_string(intent_id) << ", ağırlıklar güncellenemedi."); // GÜNCELLENDİ
    }
}

void IntentAnalyzer::update_action_success_score(UserIntent intent_id, AIAction action, float score_change) {
    auto it = intent_templates.find(intent_id);
    if (it != intent_templates.end()) {
        it->second.action_success_scores[action] += score_change;
        it->second.action_success_scores[action] = std::max(0.0f, std::min(1.0f, it->second.action_success_scores[action]));
        LOG_DEFAULT(LogLevel::DEBUG, "IntentAnalyzer: Niyet '" << CerebrumLux::intent_to_string(intent_id) << "' için eylem '" << CerebrumLux::action_to_string(action) << "' başarı puanı güncellendi."); // GÜNCELLENDİ
    } else {
        LOG_DEFAULT(LogLevel::WARNING, "IntentAnalyzer: Niyet şablonu bulunamadı: " << CerebrumLux::intent_to_string(intent_id) << ", eylem başarı puanı güncellenemedi."); // GÜNCELLENDİ
    }
}

std::vector<float> IntentAnalyzer::get_intent_weights(UserIntent intent_id) const {
    auto it = intent_templates.find(intent_id);
    if (it != intent_templates.end()) {
        return it->second.weights;
    }
    return {}; // Boş vektör döndür
}

void IntentAnalyzer::report_learning_performance(UserIntent intent_id, float implicit_feedback_avg, float explicit_feedback_avg) {
    LOG_DEFAULT(LogLevel::INFO, "IntentAnalyzer: Niyet '" << CerebrumLux::intent_to_string(intent_id) << "' için öğrenme performansı raporu - İmplicit: " << implicit_feedback_avg << ", Explicit: " << explicit_feedback_avg); // GÜNCELLENDİ
}

float IntentAnalyzer::calculate_cosine_similarity(const std::vector<float>& vec1, const std::vector<float>& vec2) const {
    if (vec1.empty() || vec1.size() != vec2.size()) {
        return 0.0f;
    }

    float dot_product = 0.0f;
    float norm1 = 0.0f;
    float norm2 = 0.0f;

    for (size_t i = 0; i < vec1.size(); ++i) {
        dot_product += vec1[i] * vec2[i];
        norm1 += vec1[i] * vec1[i];
        norm2 += vec2[i] * vec2[i];
    }

    if (norm1 == 0.0f || norm2 == 0.0f) {
        return 0.0f;
    }

    return dot_product / (std::sqrt(norm1) * std::sqrt(norm2));
}

} // namespace CerebrumLux