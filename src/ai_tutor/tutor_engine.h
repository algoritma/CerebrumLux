#ifndef TUTOR_ENGINE_H
#define TUTOR_ENGINE_H

#include <string>

class TutorEngine {
public:
    std::string generateEvaluation(const std::string& studentAnswer);
    std::string generateTask(const std::string& sectionTitle);
};

#endif // TUTOR_ENGINE_H
