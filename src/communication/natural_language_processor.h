#ifndef CEREBRUM_LUX_NATURAL_LANGUAGE_PROCESSOR_H
#define CEREBRUM_LUX_NATURAL_LANGUAGE_PROCESSOR_H

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include "../core/enums.h"
#include "../core/utils.h"
#include "../planning_execution/goal_manager.h"

class NaturalLanguageProcessor {
public:
    explicit NaturalLanguageProcessor(GoalManager& goal_manager_ref);
    NaturalLanguageProcessor(); // For tooling

    UserIntent infer_intent_from_text(const std::string& user_input) const;
    AbstractState infer_state_from_text(const std::string& user_input) const;

    std::string generate_response_text(
        UserIntent current_intent,
        AbstractState current_abstract_state,
        AIGoal current_goal,
        const DynamicSequence& sequence,
        const std::vector<std::string>& relevant_keywords = {}
    ) const;

    void update_model(const std::string& observed_text, UserIntent true_intent, const std::vector<float>& latent_cryptofig);

    void load_model(const std::string& path);
    void save_model(const std::string& path) const;

    std::string predict_intent(const std::string& input);
    
    // 🔹 Incremental training fonksiyonu (enum uyumlu)
    void trainIncremental(const std::string& input, const std::string& expected_intent);

private:
    GoalManager* goal_manager; // Changed to pointer
    std::map<UserIntent, std::vector<std::string>> intent_keyword_map;
    std::map<AbstractState, std::vector<std::string>> state_keyword_map;
    mutable std::map<UserIntent, std::vector<float>> intent_cryptofig_weights;
    float online_learning_rate = 0.05f;
    mutable std::mutex model_mutex;

    static std::string to_lower_copy(const std::string& s);
    static bool contains_keyword(const std::string& lower_text, const std::string& lower_keyword);
    float cryptofig_score_for_intent(UserIntent intent, const std::vector<float>& latent_cryptofig) const;
    UserIntent rule_based_intent_guess(const std::string& lower_text) const;
    AbstractState rule_based_state_guess(const std::string& lower_text) const;
    std::string fallback_response_for_intent(UserIntent intent, AbstractState state, const DynamicSequence& sequence) const;
};

#endif // CEREBRUM_LUX_NATURAL_LANGUAGE_PROCESSOR_H