#include "tutor_engine.h"
#include "curriculum.h" // For GLOBAL_CURRICULUM

std::string TutorEngine::generateEvaluation(const std::string& studentAnswer) {
    // STUB - placeholder logic
    return "Evaluation: " + studentAnswer;
}

std::string TutorEngine::generateTask(const std::string& sectionTitle) {
    auto* sec = CerebrumLux::GLOBAL_CURRICULUM.getSection(sectionTitle);
    if (!sec) {
        return "No section found.";
    }

    if (sec->sample_tasks.empty()) {
        return "No tasks available in this section.";
    }

    // Return the first sample task for now
    return sec->sample_tasks[0];
}
