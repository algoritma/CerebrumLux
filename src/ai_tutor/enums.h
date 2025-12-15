#ifndef AI_TUTOR_ENUMS_H
#define AI_TUTOR_ENUMS_H

#include <string>

namespace CerebrumLux {

enum class TeachingStyle {
    SOCRATIC,
    ERROR_DRIVEN,
    EXAMPLE_FIRST,
    DIRECT,
    MICRO_STEPS,
    UNKNOWN
};

inline std::string to_string(TeachingStyle style) {
    switch (style) {
        case TeachingStyle::SOCRATIC: return "SOCRATIC";
        case TeachingStyle::ERROR_DRIVEN: return "ERROR_DRIVEN";
        case TeachingStyle::EXAMPLE_FIRST: return "EXAMPLE_FIRST";
        case TeachingStyle::DIRECT: return "DIRECT";
        case TeachingStyle::MICRO_STEPS: return "MICRO_STEPS";
        case TeachingStyle::UNKNOWN: return "UNKNOWN";
    }
    return "UNKNOWN"; // Should not happen
}

enum class StudentLevel {
    BEGINNER,
    INTERMEDIATE,
    ADVANCED,
    UNKNOWN
};

inline std::string to_string(StudentLevel level) {
    switch (level) {
        case StudentLevel::BEGINNER: return "BEGINNER";
        case StudentLevel::INTERMEDIATE: return "INTERMEDIATE";
        case StudentLevel::ADVANCED: return "ADVANCED";
        case StudentLevel::UNKNOWN: return "UNKNOWN";
    }
    return "UNKNOWN"; // Should not happen
}

enum class LearningOutcome {
    NO_PROGRESS,
    PARTIAL_PROGRESS,
    CLEAR_PROGRESS,
    CONFUSION_INCREASED,
    UNKNOWN
};

inline std::string to_string(LearningOutcome outcome) {
    switch (outcome) {
        case LearningOutcome::NO_PROGRESS: return "NO_PROGRESS";
        case LearningOutcome::PARTIAL_PROGRESS: return "PARTIAL_PROGRESS";
        case LearningOutcome::CLEAR_PROGRESS: return "CLEAR_PROGRESS";
        case LearningOutcome::CONFUSION_INCREASED: return "CONFUSION_INCREASED";
        case LearningOutcome::UNKNOWN: return "UNKNOWN";
    }
    return "UNKNOWN"; // Should not happen
}

} // namespace CerebrumLux

#endif // AI_TUTOR_ENUMS_H

