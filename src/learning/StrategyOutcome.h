#ifndef STRATEGY_OUTCOME_H
#define STRATEGY_OUTCOME_H

#include <string>
#include <map>
#include <vector>
#include "../ai_tutor/enums.h" // For TeachingStyle
#include "../external/nlohmann/json.hpp" // For nlohmann::json

namespace CerebrumLux {

struct StrategyOutcome {
    TeachingStyle style;
    float delta_correctness;
    float delta_clarity;
    float delta_efficiency;
    float retention_score; // sonraki ders performansı
};

inline void to_json(nlohmann::json& j, const StrategyOutcome& o) {
    j = nlohmann::json{
        {"style", to_string(o.style)},
        {"delta_correctness", o.delta_correctness},
        {"delta_clarity", o.delta_clarity},
        {"delta_efficiency", o.delta_efficiency},
        {"retention_score", o.retention_score}
    };
}

inline void from_json(const nlohmann::json& j, StrategyOutcome& o) {
    // This is a bit more complex due to the enum
    std::string style_str = j.at("style").get<std::string>();
    if (style_str == "SOCRATIC") o.style = TeachingStyle::SOCRATIC;
    else if (style_str == "ERROR_DRIVEN") o.style = TeachingStyle::ERROR_DRIVEN;
    else if (style_str == "EXAMPLE_FIRST") o.style = TeachingStyle::EXAMPLE_FIRST;
    else if (style_str == "DIRECT") o.style = TeachingStyle::DIRECT;
    else if (style_str == "MICRO_STEPS") o.style = TeachingStyle::MICRO_STEPS;
    else o.style = TeachingStyle::UNKNOWN;
    
    j.at("delta_correctness").get_to(o.delta_correctness);
    j.at("delta_clarity").get_to(o.delta_clarity);
    j.at("delta_efficiency").get_to(o.delta_efficiency);
    j.at("retention_score").get_to(o.retention_score);
}

} // namespace CerebrumLux


#endif // STRATEGY_OUTCOME_H