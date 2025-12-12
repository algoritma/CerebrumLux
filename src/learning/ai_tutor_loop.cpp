#include "ai_tutor_loop.h"
#include "brain/llm_engine.h" // LLMEngine tanımı burada gerekli
#include "../external/nlohmann/json.hpp" // JSON kütüphanesi yolu
#include <sstream>
#include <iostream>

using json = nlohmann::json;

// -- Yardımcı Fonksiyonlar --

static std::string generateTeacherPrompt(const TeacherStudentFrame& frame) {
    json j;
    j["teacher_role"] = frame.teacher_role;
    j["student_role"] = frame.student_role;

    // Müfredatı JSON'a ekle
    for (const auto& pair : frame.curriculum.sections) {
        j["curriculum"][pair.first]["topics"] = pair.second.topics;
        j["curriculum"][pair.first]["difficulty"] = pair.second.difficulty;
    }
    
    // Önceki değerlendirmeyi bağlam (context) olarak ekle
    if (!frame.evaluation.feedback.empty()) {
        j["previous_evaluation"]["feedback"] = frame.evaluation.feedback;
        for (const auto& pair : frame.evaluation.scores) {
            j["previous_evaluation"]["scores"][pair.first] = pair.second;
        }
    }

    std::stringstream ss;
    ss << "You are the Teacher AI.\n"
       << "Your job: Teach the Student AI according to the curriculum and its previous performance.\n"
       << "Based on the provided context, design a new, single, concise task (lesson) for the student.\n"
       << "The lesson should have a clear 'goal', a 'task', and optional 'examples'.\n"
       << "Return ONLY a valid JSON object with the fields: { \"goal\": string, \"task\": string, \"examples\": [string] }.\n"
       << "Do not include markdown formatting like ```json ... ```.\n\n"
       << "Current Context:\n" << j.dump(4) << "\n";

    return ss.str();
}

static std::string generateEvaluationPrompt(const TeacherStudentFrame& frame) {
    json j;
    j["student_response"] = frame.student_response;
    j["lesson"] = {
        {"goal", frame.lesson.goal},
        {"task", frame.lesson.task}
    };

    // Puanlama kriterlerini hazırla
    std::vector<std::string> score_keys;
    for (const auto& pair : frame.curriculum.sections) {
        score_keys.push_back("score_" + pair.first);
    }
    score_keys.push_back("score_reasoning");

    std::stringstream ss;
    ss << "You are the Teacher AI.\n"
       << "Evaluate the student's response based on the lesson's goal and task.\n"
       << "Provide a score (0.0 to 1.0) for each field: " << json(score_keys).dump() << ".\n"
       << "Also provide constructive 'feedback' for the student.\n"
       << "Return ONLY a valid JSON object: { \"scores\": { \"key\": value, ... }, \"feedback\": \"string\" }.\n"
       << "Do not include markdown formatting.\n\n"
       << "Evaluation Context:\n" << j.dump(4) << "\n";

    return ss.str();
}

// -- Ana Döngü --

TeacherStudentFrame runTutorLoop(CerebrumLux::LLMEngine* teacherModel,
                                 CerebrumLux::LLMEngine* studentModel,
                                 const TeacherStudentFrame& lastFrame)
{
    TeacherStudentFrame newFrame = lastFrame;

    // 1. ADIM: Öğretmen yeni ders üretir
    std::string teacherPrompt = generateTeacherPrompt(lastFrame);
    
    if (!teacherModel || !teacherModel->is_model_loaded()) {
        newFrame.evaluation.feedback = "ERROR: Teacher LLM model is not loaded.";
        return newFrame;
    }
    
    std::string lessonJson = teacherModel->generate(teacherPrompt);

    // JSON Temizleme (Markdown temizliği gerekebilir)
    // Basitçe: ilk '{' ve son '}' arasını alabiliriz, burada direkt parse deniyoruz.
    try {
        auto j = json::parse(lessonJson);
        newFrame.lesson.goal = j.value("goal", "No goal specified.");
        newFrame.lesson.task = j.value("task", "No task specified.");
        newFrame.lesson.examples = j.value("examples", std::vector<std::string>{});
        // Feedback'i temizle, yeni döngü başlıyor
        newFrame.evaluation.feedback.clear(); 
        newFrame.evaluation.scores.clear();
    } catch (const std::exception& e) {
        newFrame.evaluation.feedback = "CRITICAL: Teacher failed to generate valid JSON lesson. Raw: " + lessonJson;
        // Bir önceki dersi koru
        return newFrame;
    }

    // 2. ADIM: Öğrenci cevap verir
    std::string studentPrompt = "Instruction:\n"
                              "Goal: " + newFrame.lesson.goal + "\n"
                              "Task: " + newFrame.lesson.task + "\n";
    if (!newFrame.lesson.examples.empty()) {
        studentPrompt += "Example: " + newFrame.lesson.examples[0] + "\n";
    }
    studentPrompt += "\nResponse:";
    
    if (!studentModel || !studentModel->is_model_loaded()) {
        newFrame.student_response = "ERROR: Student LLM not loaded.";
        return newFrame;
    }
    
    newFrame.student_response = studentModel->generate(studentPrompt);
    
    if (newFrame.student_response.empty()) {
        newFrame.student_response = "[Empty Response]";
    }

    // 3. ADIM: Öğretmen değerlendirir
    std::string evalPrompt = generateEvaluationPrompt(newFrame);
    std::string evalJson = teacherModel->generate(evalPrompt);

    try {
        auto j = json::parse(evalJson);
        newFrame.evaluation.feedback = j.value("feedback", "No feedback provided.");
        if (j.contains("scores") && j["scores"].is_object()) {
            newFrame.evaluation.scores.clear();
            for (auto& [key, value] : j["scores"].items()) {
                if (value.is_number()) {
                    newFrame.evaluation.scores[key] = value.get<float>();
                }
            }
        }
    } catch (const std::exception& e) {
        newFrame.evaluation.feedback = "CRITICAL: Teacher failed to generate valid JSON evaluation. Raw: " + evalJson;
    }

    return newFrame;
}