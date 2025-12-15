#ifndef STUDENT_LEVEL_EVALUATOR_H
#define STUDENT_LEVEL_EVALUATOR_H

#include <vector>
#include "enums.h" // StudentLevel için
#include "teacher_evaluator.h" // EvaluationResult için

namespace CerebrumLux {
namespace AITutor {

class StudentLevelEvaluator {
public:
    // Öğrenci seviyesini geçmiş değerlendirme sonuçlarından çıkarır
    static StudentLevel infer_student_level(const std::vector<EvaluationResult>& history);

    // Öğrenme sonucunu değerlendirir
    static LearningOutcome evaluate_outcome(const EvaluationResult& last_evaluation, StudentLevel current_level);
};

} // namespace AITutor
} // namespace CerebrumLux

#endif // STUDENT_LEVEL_EVALUATOR_H
