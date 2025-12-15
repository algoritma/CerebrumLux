#include "student_ai.h"
#include "llama_adapter.h" // Added for LlamaAdapter
#include "../core/logger.h" // LOG_DEFAULT için

namespace CerebrumLux { // Add CerebrumLux namespace

// Constructor implementation
StudentAI::StudentAI() : learningStore("knowledge/learning.log", "knowledge/behavior.json") {
    behaviorProfile = learningStore.loadBehaviorProfile();
    behaviorProfile.clamp(); // Ensure values are within bounds
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "StudentAI: Initialized. Behavior Profile - Politeness: " << behaviorProfile.politeness << ", Verbosity: " << behaviorProfile.verbosity);
}

// Helper to build a LLaMA-friendly prompt based on current behavior profile
std::string StudentAI::buildPrompt(const std::string& userText) {
    std::ostringstream ss;
    ss << "You are CerebrumLux student. Your current politeness level is " << behaviorProfile.politeness * 100 << "% and verbosity level is " << behaviorProfile.verbosity * 100 << "%.\n";
    ss << "Respond to the following message, maintaining your current behavior profile:\n";
    ss << "User says: " << userText;
    return ss.str();
}

std::string StudentAI::respond(const std::string& input) {
    std::string prompt = buildPrompt(input);
    try {
        return CerebrumLux::LlamaAdapter::infer_sync(prompt);
    } catch (const std::exception& ex) {
        return "Student AI inference error: " + std::string(ex.what());
    }
}

void StudentAI::adjustBehavior(const CerebrumLux::EvaluationResult& eval) {
    // Örnek: Değerlendirme sonuçlarına göre davranış profilini ayarla
    // Burada daha sofistike bir öğrenme algoritması olabilir.
    // Şimdilik basit bir heuristic kullanılıyor.

    float overall_score_normalized = eval.score_overall / 100.0f;
    float politeness_score = eval.score_conversation / 100.0f; // Konuşma skoru nezaketi temsil edebilir

    if (politeness_score < behaviorProfile.politeness - 0.1f) {
        behaviorProfile.politeness = std::min(1.0f, behaviorProfile.politeness + 0.05f); // Daha nazik olmaya çalış
        LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "StudentAI: Nezaket seviyesi düşüktü, artırıldı. Yeni nezaket: " << behaviorProfile.politeness);
    } else if (politeness_score > behaviorProfile.politeness + 0.1f) {
        behaviorProfile.politeness = std::max(0.0f, behaviorProfile.politeness - 0.02f); // Çok nazikse biraz düşür
        LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "StudentAI: Nezaket seviyesi yüksekti, azaltıldı. Yeni nezaket: " << behaviorProfile.politeness);
    }

    // Geribildirimin uzunluğuna göre verbosity ayarı
    if (eval.feedback.length() > 100 && behaviorProfile.verbosity > 0.3f) { // Çok uzun feedback geldiyse verbosity'i düşür
        behaviorProfile.verbosity = std::max(0.0f, behaviorProfile.verbosity - 0.05f);
        LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "StudentAI: Geri bildirim çok uzundu, verbosity azaltıldı. Yeni verbosity: " << behaviorProfile.verbosity);
    } else if (eval.feedback.length() < 30 && behaviorProfile.verbosity < 0.7f) { // Çok kısa feedback geldiyse verbosity'i artır
        behaviorProfile.verbosity = std::min(1.0f, behaviorProfile.verbosity + 0.05f);
        LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "StudentAI: Geri bildirim çok kısaydı, verbosity artırıldı. Yeni verbosity: " << behaviorProfile.verbosity);
    }

    behaviorProfile.clamp();
    learningStore.saveBehaviorProfile(behaviorProfile); // Güncellenmiş profili kaydet
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "StudentAI: Davranış profili değerlendirme sonucuna göre ayarlandı ve kaydedildi.");
}

} // namespace CerebrumLux
