// teacher_evaluator.h
// Teacher evaluator: uses LLaMA via LlamaAdapter to evaluate a student's response
// and returns a structured EvaluationResult.

#ifndef TEACHER_EVALUATOR_H
#define TEACHER_EVALUATOR_H

#include <string>
#include <optional>
#include <nlohmann/json.hpp> // Required for EvaluationParser
#include "../core/logger.h" // For LOG_DEFAULT
#include "../core/enums.h" // For CerebrumLux::LogLevel

namespace CerebrumLux { // YENİ: CerebrumLux namespace'i eklendi

struct EvaluationResult {
    int score_cxx = 0;
    int score_conversation = 0;
    int score_overall = 0;
    std::string feedback;
    std::string raw_json; // full teacher output (for debugging)
};

class EvaluationParser {
public:
    static float extractScore(const std::string& evaluation_json) {
        try {
            nlohmann::json j = nlohmann::json::parse(evaluation_json);
            return j.value("score_overall", 0) / 100.0f; // 0-100 arası skoru 0.0-1.0 arasına çevir
        } catch (const std::exception& e) {
            LOG_DEFAULT(CerebrumLux::LogLevel::ERR_CRITICAL, "EvaluationParser: JSON parse hatası: " << e.what());
            return 0.0f;
        }
    }
     static EvaluationResult parseEvaluationResult(const std::string& evaluation_json) {
        try {
            nlohmann::json j = nlohmann::json::parse(evaluation_json);
            EvaluationResult res;
            res.score_cxx = j.value("score_cxx", 0);
            res.score_conversation = j.value("score_conversation", 0);
            res.score_overall = j.value("score_overall", 0);
            res.feedback = j.value("feedback", "");
            res.raw_json = evaluation_json;
            return res;
        } catch (const std::exception& e) {
            LOG_DEFAULT(CerebrumLux::LogLevel::ERR_CRITICAL, "EvaluationParser: EvaluationResult JSON parse hatası: " << e.what());
            return EvaluationResult(); // Varsayılan değerlerle boş sonuç dön
        }
    }
};

class TeacherEvaluator {
public:
    // Ask teacher LLM to evaluate; returns EvaluationResult on success.
    // This calls CerebrumLux::LlamaAdapter::infer_sync internally.
    static std::optional<EvaluationResult> evaluate_response(
        const std::string &lesson_context,
        const std::string &student_response
    );

    // Utility: constructs the JSON-scoring prompt (human-readable)
    static std::string build_evaluation_prompt(
        const std::string &lesson_context,
        const std::string &student_response
    );
};

} // namespace CerebrumLux // YENİ: CerebrumLux namespace'i kapatıldı

#endif // TEACHER_EVALUATOR_H
