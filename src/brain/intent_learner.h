#ifndef INTENT_LEARNER_H
#define INTENT_LEARNER_H

#include <string>
#include <vector>
#include <map>
#include <deque> // Tarihçe için
#include "../core/enums.h" // UserIntent, AbstractState, AIAction, SensorType için
#include "../data_models/dynamic_sequence.h" // DynamicSequence için
#include "../sensors/atomic_signal.h" // AtomicSignal için
#include "../brain/intent_analyzer.h" // IntentAnalyzer için
#include "../communication/suggestion_engine.h" // SuggestionEngine için
#include "../user/user_profile_manager.h" // UserProfileManager için
#include "../core/utils.h" // MessageQueue için

namespace CerebrumLux { // IntentLearner sınıfı bu namespace içine alınacak

class IntentLearner {
public:
    IntentLearner(IntentAnalyzer& analyzer, SuggestionEngine& suggester, UserProfileManager& user_profile_manager);

    // Öğrenme hızını ayarlar
    void set_learning_rate(float rate);
    float get_learning_rate() const { return learning_rate; }

    // Geri bildirim işleme
    void process_feedback(const DynamicSequence& sequence, UserIntent predicted_intent,
                          const std::deque<AtomicSignal>& recent_signals);

    // Açık geri bildirim işleme (kullanıcı onayı gibi)
    virtual void process_explicit_feedback(UserIntent predicted_intent, AIAction action, bool approved, const DynamicSequence& sequence, AbstractState current_abstract_state);

    // Tarihçeye erişim için
    const std::map<UserIntent, std::deque<float>>& get_implicit_feedback_history() const { return implicit_feedback_history; }
    size_t get_feedback_history_size() const { return feedback_history_size; }
    bool get_implicit_feedback_for_intent(UserIntent intent_id, std::deque<float>& history_out) const;

    // Soyut durumu çıkarım
    virtual AbstractState infer_abstract_state(const std::deque<AtomicSignal>& recent_signals);

private:
    IntentAnalyzer& intent_analyzer;
    SuggestionEngine& suggestion_engine;
    UserProfileManager& user_profile_manager;

    float learning_rate;
    size_t feedback_history_size; // Geri bildirim tarihçesinin boyutu

    MessageQueue messageQueue; // Dahili mesaj kuyruğu

    std::map<UserIntent, std::deque<float>> implicit_feedback_history;
    std::map<UserIntent, std::deque<float>> explicit_feedback_history; // Şu an kullanılmıyor olabilir, ancak yapılandırıldı

    // Sensör ve performans katkı analizi
    std::map<SensorType, float> sensor_performance_contribution; // Her sensörün genel performansa katkısı
    std::map<SensorType, int> sensor_feedback_count; // Her sensörden gelen feedback sayısı

    // Yardımcı fonksiyonlar
    void evaluate_implicit_feedback(UserIntent current_intent, AbstractState current_abstract_state);
    void adjust_template(UserIntent intent_id, const DynamicSequence& sequence, float feedback_strength);
    void adjust_action_score(UserIntent intent_id, AIAction action, float score_change);
    float calculate_sensor_contribution(SensorType type) const;
};

} // namespace CerebrumLux

#endif // INTENT_LEARNER_H
