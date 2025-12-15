#ifndef LEARNING_STATE_H
#define LEARNING_STATE_H

#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>

namespace CerebrumLux {

struct LearningState {
    std::string domain;        // conversation, reasoning, coding
    std::string skill;         // politeness, clarity, correctness
    float score_avg = 0.0f;
    int samples = 0;
    bool stabilized = false;

    // JSON'dan dönüştürme
    friend void to_json(nlohmann::json& j, const LearningState& p) {
        j = nlohmann::json{{"domain", p.domain}, {"skill", p.skill},
                           {"score_avg", p.score_avg}, {"samples", p.samples},
                           {"stabilized", p.stabilized}};
    }

    // JSON'dan yükleme
    friend void from_json(const nlohmann::json& j, LearningState& p) {
        j.at("domain").get_to(p.domain);
        j.at("skill").get_to(p.skill);
        j.at("score_avg").get_to(p.score_avg);
        j.at("samples").get_to(p.samples);
        j.at("stabilized").get_to(p.stabilized);
    }
};

} // namespace CerebrumLux

#endif // LEARNING_STATE_H