#ifndef TEACHER_AI_H
#define TEACHER_AI_H

#include <string>
#include <vector>
#include <map>
#include <algorithm> // For std::max_element

#include "tutor_engine.h"
#include "enums.h" // For TeachingStyle, StudentLevel
#include "../core/enums.h" // For UserIntent
#include "../learning/StrategyOutcome.h" // For StrategyOutcome

namespace CerebrumLux { // Add CerebrumLux namespace

struct PedagogicalRule {
    UserIntent intent;
    StudentLevel level;
    TeachingStyle style;
};

static const std::vector<PedagogicalRule> PEDAGOGY_RULES = {
    { UserIntent::Programming, StudentLevel::BEGINNER,     TeachingStyle::MICRO_STEPS },
    { UserIntent::Programming, StudentLevel::INTERMEDIATE, TeachingStyle::ERROR_DRIVEN },
    { UserIntent::Programming, StudentLevel::ADVANCED,     TeachingStyle::SOCRATIC },

    { UserIntent::Communication, StudentLevel::BEGINNER, TeachingStyle::EXAMPLE_FIRST },
    { UserIntent::Communication, StudentLevel::INTERMEDIATE, TeachingStyle::EXAMPLE_FIRST },
    { UserIntent::Communication, StudentLevel::ADVANCED, TeachingStyle::SOCRATIC },

    { UserIntent::Research, StudentLevel::BEGINNER, TeachingStyle::DIRECT },
    { UserIntent::Research, StudentLevel::INTERMEDIATE, TeachingStyle::DIRECT },
    { UserIntent::Research, StudentLevel::ADVANCED, TeachingStyle::SOCRATIC },

    { UserIntent::RequestInformation, StudentLevel::UNKNOWN, TeachingStyle::DIRECT }, // Varsayılan veya bilinmeyen seviye için
    { UserIntent::CreativeWork, StudentLevel::UNKNOWN, TeachingStyle::MICRO_STEPS } // Varsayılan veya bilinmeyen seviye için
};

struct TeacherAutoEval {
    float relevance;
    float coherence;
    float pedagogical_quality;
    float overall() const {
        // RLHF / TeacherAI geri besleme skoru
        return (relevance + coherence + pedagogical_quality) / 3.0f;
    }
};

class TeacherAI {
public:
    std::string teach(const std::string& sectionName);
    std::string evaluate(const std::string& studentAnswer);

    // RLHF için yapılandırılmış otomatik değerlendirme
    TeacherAutoEval auto_evaluate_chat(
        const std::string& user_input,
        const std::string& assistant_reply
    );

    // Meta-öğretmen Karar Mekanizması
    static TeachingStyle select_best_strategy(const std::vector<StrategyOutcome>& history);

    // Dinamik Stil Seçimi - Artık öğrenci seviyesini de içeriyor
    static TeachingStyle resolve_teaching_style(
        UserIntent intent,
        StudentLevel level, // Yeni parametre
        const std::vector<StrategyOutcome>& history
    );

    // Yeni: Pedagojik stil ve intent tipine göre ders üretir
    std::string generate_lesson(TeachingStyle style, UserIntent intent, const std::string& user_input);

private:
    TutorEngine engine;
};

} // namespace CerebrumLux

#endif
