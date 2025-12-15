#ifndef STUDENT_AI_H
#define STUDENT_AI_H

#include <string>
#include "../learning/BehaviorProfile.h" // YENİ: Öğrenme davranış profili için
#include "../learning/LearningStore.h"    // YENİ: Kalıcı öğrenme depolaması için
#include "teacher_evaluator.h" // YENİ: EvaluationResult için

namespace CerebrumLux { // Add CerebrumLux namespace

class StudentAI {
public:
    StudentAI(); // YENİ: Varsayılan kurucu eklendi

    std::string respond(const std::string& input);

    // YENİ: Değerlendirme sonuçlarına göre davranışı ayarla
    void adjustBehavior(const CerebrumLux::EvaluationResult& eval);

private:
    CerebrumLux::BehaviorProfile behaviorProfile;
    CerebrumLux::LearningStore learningStore;

    // YENİ: Prompt oluşturma yardımcısı
    std::string buildPrompt(const std::string& userText);
};

} // namespace CerebrumLux

#endif
