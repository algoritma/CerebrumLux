#ifndef CEREBRUM_LUX_INTENT_LEARNER_H
#define CEREBRUM_LUX_INTENT_LEARNER_H

#include <deque>   // For std::deque
#include <map>     // For std::map
#include <vector>  // For std::vector
#include "../core/enums.h"         // Enumlar için
#include "../data_models/dynamic_sequence.h" // DynamicSequence için ileri bildirim
#include "../sensors/atomic_signal.h" // AtomicSignal için (evaluate_implicit_feedback içinde kullanılır)
#include "intent_analyzer.h"       // IntentAnalyzer için ileri bildirim
#include "../core/utils.h"
#include "../communication/suggestion_engine.h" // SuggestionEngine için eklendi
#include "../user/user_profile_manager.h" // UserProfileManager için eklendi

// İleri bildirimler
struct DynamicSequence;
class IntentAnalyzer;
class SuggestionEngine;
class UserProfileManager; // UserProfileManager için ileri bildirim eklendi

// *** IntentLearner: Geri bildirim yoluyla niyet tahminlerini gelistirir ***
class IntentLearner {
public:
    // Kurucuya SuggestionEngine ve UserProfileManager referansları eklendi
    IntentLearner(IntentAnalyzer& analyzer_ref, SuggestionEngine& suggester_ref, UserProfileManager& user_profile_manager_ref); 
    
    //mesaj kuyruğunu
    void adjust_learning_rate(float new_learning_rate);
    void processMessages();
    void update_action_success_score(UserIntent intent, AIAction next_action, float success_score);


    void process_feedback(const DynamicSequence& sequence, UserIntent predicted_intent, 
                          const std::deque<AtomicSignal>& recent_signals);

    // current_abstract_state parametresi eklendi
    virtual void process_explicit_feedback(UserIntent predicted_intent, AIAction action, bool approved, const DynamicSequence& sequence, AbstractState current_abstract_state);

    void self_adjust_learning_rate(float adjustment_factor);

    float get_learning_rate() const { return learning_rate; }

    const std::map<UserIntent, std::deque<float>>& get_implicit_feedback_history() const { 
        return implicit_feedback_history; 
    }

    bool get_implicit_feedback_for_intent(UserIntent intent_id, std::deque<float>& history_out) const {
        auto it = implicit_feedback_history.find(intent_id);
        if (it != implicit_feedback_history.end()) {
            history_out = it->second;
            return true;
        }
        return false;
    }
    size_t get_feedback_history_size() const { return feedback_history_size; } // Getter metodu

    float get_desired_other_signals_multiplier() const { return desired_other_signals_multiplier; }

    virtual AbstractState infer_abstract_state(const std::deque<AtomicSignal>& recent_signals); // Buraya 'virtual' ekleyin
    
private:
    IntentAnalyzer& analyzer; 
    SuggestionEngine& suggester; 
    UserProfileManager& user_profile_manager; // YENİ: UserProfileManager referansı eklendi

    float learning_rate = 0.1f; // Varsayılan öğrenme oranı
    //mesaj işleme metodu
    MessageQueue messageQueue;

    void evaluate_implicit_feedback(UserIntent current_intent, AbstractState current_abstract_state);
    void adjust_template(UserIntent intent_id, const DynamicSequence& sequence, float feedback_strength);
    void adjust_action_score(UserIntent intent_id, AIAction action, float score_change); 
    
    std::map<UserIntent, std::deque<float>> implicit_feedback_history;
    std::map<UserIntent, std::deque<float>> explicit_feedback_history;
    size_t feedback_history_size = 10; 

    void evaluate_and_meta_adjust();

    std::map<SensorType, float> sensor_performance_contribution; 
    std::map<SensorType, int> sensor_feedback_count; 
    
    float desired_other_signals_multiplier = 1.0f; 

};

#endif // CEREBRUM_LUX_INTENT_LEARNER_H