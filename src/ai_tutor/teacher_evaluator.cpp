// teacher_evaluator.cpp
#include "teacher_evaluator.h"
#include "llama_adapter.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <sstream> // Required for std::ostringstream

namespace CerebrumLux { // Add CerebrumLux namespace

using json = nlohmann::json;

std::string TeacherEvaluator::build_evaluation_prompt(
    const std::string &lesson_context,
    const std::string &student_response
) {
    // Prompt requests strict JSON response with numeric fields.
    std::ostringstream ss;
    ss << "You are a strict automatic teacher evaluator.\n";
    ss << "Lesson context:\n" << lesson_context << "\n\n";
    ss << "Student response:\n" << student_response << "\n\n";
    ss << "Evaluate according to rubric and return JSON ONLY with keys:\n";
    ss << "{ \"score_cxx\": <0-100>, \"score_conversation\": <0-100>, \"score_overall\": <0-100>, \"feedback\": \"text\" }\n";
    ss << "Give concise feedback and do not output any extra commentary.\n";
    return ss.str();
}

std::optional<EvaluationResult> TeacherEvaluator::evaluate_response(
    const std::string &lesson_context,
    const std::string &student_response
) {
    std::string prompt = build_evaluation_prompt(lesson_context, student_response);
    std::string raw;
    try {
        raw = CerebrumLux::LlamaAdapter::infer_sync(prompt);
    } catch (const std::exception &ex) {
        std::cerr<<"TeacherEvaluator: inference error: "<<ex.what()<<"\n";
        return std::nullopt;
    }
    // try parse JSON in raw (robust parse: find first { ... })
    auto first = raw.find('{');
    if (first == std::string::npos) { // fallback: return as feedback
        EvaluationResult r; r.feedback = raw; r.raw_json = raw; return r;
    }
    std::string jsonpart = raw.substr(first);
    try {
        json j = json::parse(jsonpart);
        EvaluationResult res;
        res.score_cxx = j.value("score_cxx", 0);
        res.score_conversation = j.value("score_conversation", 0);
        res.score_overall = j.value("score_overall", 0);
        res.feedback = j.value("feedback", "");
        res.raw_json = jsonpart;
        return res;
    } catch (const std::exception &ex) {
        // parsing failed
        EvaluationResult r; r.feedback = raw; r.raw_json = raw; return r;
    }
}

} // namespace CerebrumLux
