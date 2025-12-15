#ifndef BEHAVIOR_PROFILE_H
#define BEHAVIOR_PROFILE_H

#include <string>
#include <algorithm> // For std::clamp
#include <nlohmann/json.hpp>

namespace CerebrumLux {

struct BehaviorProfile {
    float politeness = 0.5f;
    float verbosity  = 0.5f;
    float confidence = 0.5f;

    void clamp() {
        politeness = std::clamp(politeness, 0.0f, 1.0f);
        verbosity  = std::clamp(verbosity, 0.0f, 1.0f);
        confidence = std::clamp(confidence, 0.0f, 1.0f);
    }

    // JSON'dan dönüştürme
    friend void to_json(nlohmann::json& j, const BehaviorProfile& p) {
        j = nlohmann::json{{"politeness", p.politeness}, {"verbosity", p.verbosity},
                           {"confidence", p.confidence}};
    }

    // JSON'dan yükleme
    friend void from_json(const nlohmann::json& j, BehaviorProfile& p) {
        p.politeness = j.value("politeness", 0.5f);
        p.verbosity = j.value("verbosity", 0.5f);
        p.confidence = j.value("confidence", 0.5f);
    }
};

} // namespace CerebrumLux

#endif // BEHAVIOR_PROFILE_H