#ifndef CEREBRUM_LUX_PREDICTION_ENGINE_H
#define CEREBRUM_LUX_PREDICTION_ENGINE_H

#include <vector>  // For std::vector
#include <deque>   // For std::deque
#include <string>  // For std::wstring
#include <limits>  // For std::numeric_limits
#include <cmath>   // For std::exp
#include "../core/enums.h"         // Enumlar için
#include "../core/utils.h"         // intent_to_string için (LOG içinde kullanılır)
#include "../data_models/dynamic_sequence.h" // DynamicSequence için ileri bildirim
#include "intent_analyzer.h"       // IntentAnalyzer için ileri bildirim
#include "autoencoder.h"           // CryptofigAutoencoder::LATENT_DIM için (sadece boyut için)

// İleri bildirimler
struct DynamicSequence;
class IntentAnalyzer;
class SequenceManager; // PredictionEngine'ın constructor'ında kullanıldığı için

// *** StateNode: Evrimsel Durum Grafigi için dugum yapisi ***
struct StateNode {
    UserIntent intent;
    std::vector<float> dominant_cryptofig; // latent_cryptofig_vector boyutunda olacak
    int total_outgoing_transitions = 0; 

    StateNode(UserIntent i); // Constructor bildirimi
};


// *** StateEdge: Evrimsel Durum Grafigi için kenar yapisi ***
struct StateEdge {
    UserIntent from_intent;
    UserIntent to_intent;
    std::vector<float> transition_cryptofig_delta; // latent_cryptofig_vector boyutunda olacak
    float transition_probability; 
    long long last_observed_us; 
    int observation_count; 

    StateEdge(UserIntent from, UserIntent to); // Constructor bildirimi
};


// *** PredictionEngine: Tahminci zeka için durum grafigini kullanir ***
class PredictionEngine { 
public:
    PredictionEngine(IntentAnalyzer& analyzer_ref, SequenceManager& manager_ref);

    virtual UserIntent predict_next_intent(UserIntent previous_intent, const DynamicSequence& current_sequence) const; // latent_cryptofig_vector kullanacak

    void update_state_graph(UserIntent previous_intent, UserIntent current_intent, const DynamicSequence& sequence); // latent_cryptofig_vector kullanacak
    
    void save_state_graph(const std::wstring& filename) const;
    void load_state_graph(const std::wstring& filename);

    float query_intent_probability(UserIntent target_intent, const DynamicSequence& current_sequence) const;
    void learn_time_patterns(const std::deque<AtomicSignal>& signal_buffer, UserIntent current_intent);

private:
    IntentAnalyzer& analyzer;
    SequenceManager& manager; 

    std::vector<StateNode> state_nodes; 
    std::vector<StateEdge> state_edges; 

    void initialize_state_graph(); 
    StateEdge* find_or_create_edge(UserIntent from_intent, UserIntent to_intent); 

    float calculate_euclidean_distance(const std::vector<float>& vec1, const std::vector<float>& vec2) const;
};

#endif // CEREBRUM_LUX_PREDICTION_ENGINE_H