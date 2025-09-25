#ifndef INTENT_TEMPLATE_H
#define INTENT_TEMPLATE_H

#include <string>
#include <vector>
#include <map>
#include "../core/enums.h" // UserIntent, AIAction için

namespace CerebrumLux { // IntentTemplate struct'ı bu namespace içine alınacak

// Her bir UserIntent için bir şablon
struct IntentTemplate {
    UserIntent id;
    std::vector<float> weights; // Bu niyetin özellik uzayındaki temsilcisi
    float confidence_threshold; // Bu niyeti tanımak için gereken minimum güven
    std::map<AIAction, float> action_success_scores; // Bu niyetle ilişkili eylemlerin başarı puanları

        // YENİ EKLENDİ: Varsayılan kurucu
    IntentTemplate() 
        : id(UserIntent::Undefined), 
          confidence_threshold(0.7f) {
        // weights ve action_success_scores default olarak boş/sıfır başlatılır.
        // Gerekirse burada varsayılan değerler atanabilir.
    }

    IntentTemplate(UserIntent intent_id, const std::vector<float>& initial_weights)
        : id(intent_id), weights(initial_weights), confidence_threshold(0.7f) {
        // Varsayılan eylem başarı puanlarını başlat
        action_success_scores[AIAction::None] = 0.5f;
        action_success_scores[AIAction::RespondToUser] = 0.7f;
        // ... diğer aksiyonlar için başlangıç puanları
    }

    // JSON serileştirme desteği için (eğer kullanılıyorsa)
    // NLOHMANN_DEFINE_TYPE_INTRUSIVE(IntentTemplate, id, weights, confidence_threshold, action_success_scores)
};

} // namespace CerebrumLux

#endif // INTENT_TEMPLATE_H
