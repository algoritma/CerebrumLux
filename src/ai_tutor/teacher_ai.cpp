#include "teacher_ai.h"
#include "curriculum.h"
#include "teacher_evaluator.h" // Added for TeacherEvaluator
#include "student_level_evaluator.h" // Added for StudentLevelEvaluator
#include <nlohmann/json.hpp>   // Added for JSON handling
#include "../core/utils.h" // CerebrumLux::to_string için

namespace CerebrumLux { // Add CerebrumLux namespace

std::string TeacherAI::teach(const std::string& sectionName) {
    return engine.generateTask(sectionName);
}

std::string TeacherAI::evaluate(const std::string& studentAnswer) {
    // Lesson context: Placeholder, should be pulled from actual lesson data if available.
    auto res = TeacherEvaluator::evaluate_response("lesson context", studentAnswer);
    if (res) {
        nlohmann::json j;
        j["score_overall"] = res->score_overall;
        j["feedback"] = res->feedback;
        j["raw_json"] = res->raw_json; // Include raw JSON for debugging/transparency
        return j.dump();
    }
    return "Evaluation error";
}

TeachingStyle TeacherAI::select_best_strategy(const std::vector<CerebrumLux::StrategyOutcome>& history) {
    std::map<TeachingStyle, float> score;

    for (const auto& h : history) {
        score[h.style] +=
            h.delta_correctness * 0.4f +
            h.delta_clarity     * 0.3f +
            h.delta_efficiency  * 0.2f +
            h.retention_score   * 0.1f;
    }

    if (score.empty()) {
        return TeachingStyle::UNKNOWN;
    }

    return std::max_element(
        score.begin(), score.end(),
        [](auto& a, auto& b){ return a.second < b.second; }
    )->first;
}

TeachingStyle TeacherAI::resolve_teaching_style(
    UserIntent intent,
    StudentLevel level, // Yeni parametre
    const std::vector<CerebrumLux::StrategyOutcome>& history
) {
    // 1. Pedagogik kurallara göre temel stil
    TeachingStyle base = TeachingStyle::UNKNOWN; // Varsayılanı UNKNOWN yapalım
    for (const auto& r : PEDAGOGY_RULES) {
        if (r.intent == intent && (r.level == level || r.level == StudentLevel::UNKNOWN)) {
            base = r.style;
            if (r.level == level) break; // Eğer spesifik seviye kuralı varsa, onu al ve çık
        }
    }
    // Eğer spesifik bir kural bulunamazsa ve UNKNOWN seviyesi için bir kural varsa, onu kullanır.
    if (base == TeachingStyle::UNKNOWN) {
        for (const auto& r : PEDAGOGY_RULES) {
            if (r.intent == intent && r.level == StudentLevel::UNKNOWN) {
                base = r.style;
                break;
            }
        }
    }
    if (base == TeachingStyle::UNKNOWN) base = TeachingStyle::DIRECT; // Hala bulunamadıysa varsayılan

    // 2. Meta-öğretmen düzeltmesi (geçmiş tecrübeye göre)
    TeachingStyle learned = select_best_strategy(history);

    // 3. Eğer geçmişte daha iyi sonuç verdiyse override et
    // Basit bir kural: Eğer öğrenilmiş bir strateji varsa ve UNKNOWN değilse, onu tercih et
    if (learned != TeachingStyle::UNKNOWN) {
        return learned;
    }
    return base;
}

std::string TeacherAI::generate_lesson(TeachingStyle style, UserIntent intent, const std::string& user_input) {
    std::string prompt;
    // Pedagojik stil ve niyet tipine göre farklı prompt'lar oluştur
    switch (style) {
        case TeachingStyle::SOCRATIC:
            prompt = "Socratic method, intent is " + CerebrumLux::to_string(intent) + ". Based on user input '" + user_input + "', ask a probing question to guide the student's thinking.";
            break;
        case TeachingStyle::ERROR_DRIVEN:
            prompt = "Error-driven learning, intent is " + CerebrumLux::to_string(intent) + ". User input '" + user_input + "'. Provoke an error or highlight a common mistake related to this input.";
            break;
        case TeachingStyle::EXAMPLE_FIRST:
            prompt = "Example-first teaching, intent is " + CerebrumLux::to_string(intent) + ". User input '" + user_input + "'. Provide a clear example demonstrating the concept or solution.";
            break;
        case TeachingStyle::DIRECT:
            prompt = "Direct instruction, intent is " + CerebrumLux::to_string(intent) + ". User input '" + user_input + "'. Directly explain the concept or provide the answer.";
            break;
        case TeachingStyle::MICRO_STEPS:
            prompt = "Micro-steps guidance, intent is " + CerebrumLux::to_string(intent) + ". User input '" + user_input + "'. Break down the problem into very small, manageable steps.";
            break;
        case TeachingStyle::UNKNOWN:
        default:
            prompt = "Default teaching approach, intent is " + CerebrumLux::to_string(intent) + ". User input '" + user_input + "'. Explain the concept.";
            break;
    }
    // Burada LLaMA çağrısı yapılabilir veya TutorEngine kullanılabilir
    // Örneğin: return CerebrumLux::LlamaAdapter::infer_sync(prompt);
    // Şimdilik sadece prompt'u döndürüyoruz veya TutorEngine'i kullanıyoruz.
    return engine.generateTask(prompt); // TutorEngine'in generateTask'ını kullanarak genel bir görev oluştur
}

TeacherAutoEval TeacherAI::auto_evaluate_chat(
    const std::string& user_input,
    const std::string& assistant_reply
) {
    TeacherAutoEval eval{};

    // --------------------------------------------------
    // 1. Relevance – kullanıcı girdisiyle anlamsal örtüşme
    // --------------------------------------------------
    if (!user_input.empty()) {
        const auto key = user_input.substr(0, std::min<size_t>(12, user_input.size()));
        eval.relevance =
            assistant_reply.find(key) != std::string::npos ? 1.0f : 0.65f;
    } else {
        eval.relevance = 0.5f;
    }

    // --------------------------------------------------
    // 2. Coherence – cevap uzunluğu + yapı sinyali
    // --------------------------------------------------
    eval.coherence =
        (assistant_reply.size() > 80 && assistant_reply.find('.') != std::string::npos)
            ? 0.9f
            : 0.6f;

    // --------------------------------------------------
    // 3. Pedagogical quality – öğretici yapı
    // (ileride TutorEngine / rubric / embedding ile değiştirilecek)
    // --------------------------------------------------
    eval.pedagogical_quality =
        assistant_reply.find("step") != std::string::npos ||
        assistant_reply.find("example") != std::string::npos
            ? 0.9f
            : 0.7f;

    return eval;
}

} // namespace CerebrumLux

