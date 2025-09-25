#ifndef PREDICTION_ENGINE_H
#define PREDICTION_ENGINE_H

#include <string>
#include <vector>
#include <map>
#include <deque> // time_pattern_history için
#include <algorithm> // std::max için
#include "../core/enums.h" // UserIntent, AtomicSignal için
#include "../data_models/dynamic_sequence.h" // DynamicSequence için
#include "../data_models/sequence_manager.h" // SequenceManager için
#include "../sensors/atomic_signal.h" // AtomicSignal için

namespace CerebrumLux { // PredictionEngine, StateNode, StateEdge sınıfları bu namespace içine alınacak

// Durum grafiği için düğüm (niyet)
struct StateNode {
    UserIntent intent;
    std::map<UserIntent, float> transition_probabilities;

    // YENİ EKLENDİ: Varsayılan kurucu
    StateNode() : intent(UserIntent::Undefined) {} // Varsayılan niyet ile başlat

    StateNode(UserIntent i) : intent(i) {}

    bool operator<(const StateNode& other) const {
        return intent < other.intent;
    }
};

// Durum grafiği için kenar (niyetten niyete geçiş)
struct StateEdge {
    UserIntent from_intent;
    UserIntent to_intent;
    float weight; // Geçişin gücü/sıklığı
    long long last_transition_time_us; // Son geçiş zamanı

    // YENİ EKLENDİ: Varsayılan kurucu
    StateEdge()
        : from_intent(UserIntent::Undefined),
          to_intent(UserIntent::Undefined),
          weight(0.1f),
          last_transition_time_us(0) {}

    StateEdge(UserIntent from, UserIntent to)
        : from_intent(from), to_intent(to), weight(0.1f), last_transition_time_us(0) {}

    bool operator<(const StateEdge& other) const {
        if (from_intent != other.from_intent) return from_intent < other.from_intent;
        return to_intent < other.to_intent;
    }
};


class PredictionEngine {
public:
    PredictionEngine(IntentAnalyzer& analyzer, SequenceManager& seq_manager);

    virtual UserIntent predict_next_intent(UserIntent previous_intent, const DynamicSequence& current_sequence) const;
    void update_state_graph(UserIntent previous_intent, UserIntent current_intent, const DynamicSequence& sequence);
    float query_intent_probability(UserIntent target_intent, const DynamicSequence& current_sequence) const;
    void learn_time_patterns(const std::deque<AtomicSignal>& signal_buffer, UserIntent current_intent);

private:
    IntentAnalyzer& intent_analyzer;
    SequenceManager& sequence_manager;

    std::map<UserIntent, StateNode> state_graph_nodes;
    std::map<std::pair<UserIntent, UserIntent>, StateEdge> state_graph_edges;

    // Zaman deseni öğrenme
    std::map<UserIntent, std::deque<long long>> time_pattern_history; // Niyet bazında zaman aralıkları
    size_t time_pattern_history_limit;

    StateEdge* find_or_create_edge(UserIntent from_intent, UserIntent to_intent);
};

} // namespace CerebrumLux

#endif // PREDICTION_ENGINE_H