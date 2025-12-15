#include "student_level_evaluator.h"
#include <numeric> // std::accumulate için

namespace CerebrumLux {
namespace AITutor {

StudentLevel StudentLevelEvaluator::infer_student_level(const std::vector<EvaluationResult>& history) {
    if (history.empty()) {
        return StudentLevel::UNKNOWN;
    }

    float total_correctness = 0.0f;
    float total_clarity = 0.0f;

    for (const auto& r : history) {
        total_correctness += r.score_cxx; // score_cxx 0-100 arası varsayılıyor
        total_clarity += r.score_conversation; // score_conversation 0-100 arası varsayılıyor
    }

    float avg_correctness = total_correctness / history.size(); // 0-100 arası
    float avg_clarity = total_clarity / history.size(); // 0-100 arası

    // Ağırlıklı ortalama veya sadece correctness baz alınabilir
    float overall_avg_score = (avg_correctness * 0.7f) + (avg_clarity * 0.3f); // %70 doğruluk, %30 açıklık

    if (overall_avg_score < 50.0f) { // %50 altı başlangıç
        return StudentLevel::BEGINNER;
    }
    if (overall_avg_score < 80.0f) { // %50-80 arası orta
        return StudentLevel::INTERMEDIATE;
    }
    return StudentLevel::ADVANCED; // %80 üzeri ileri
}

LearningOutcome StudentLevelEvaluator::evaluate_outcome(const EvaluationResult& last_evaluation, StudentLevel current_level) {
    // Basit heuristic'ler ile öğrenme çıktısını değerlendir
    // Daha karmaşık mantık burada eklenebilir (örn. önceki seviye ile karşılaştırma, yanıt süresi vb.)

    if (last_evaluation.score_overall >= 90) { // Yüksek puan = net ilerleme
        return LearningOutcome::CLEAR_PROGRESS;
    } else if (last_evaluation.score_overall >= 60) { // Orta puan = kısmi ilerleme
        return LearningOutcome::PARTIAL_PROGRESS;
    } else if (last_evaluation.score_overall >= 30) { // Düşük puan ama sıfır değil = ilerleme yok (belki yanlış anlama)
        return LearningOutcome::NO_PROGRESS;
    } else { // Çok düşük puan = kafa karışıklığı arttı
        return LearningOutcome::CONFUSION_INCREASED;
    }
}

} // namespace AITutor
} // namespace CerebrumLux
