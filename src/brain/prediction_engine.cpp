#include "prediction_engine.h"
#include "../core/logger.h"
#include "../core/utils.h" // intent_to_string için
#include <numeric> // std::accumulate
#include <cmath>   // std::exp, std::log için

namespace CerebrumLux {

PredictionEngine::PredictionEngine(IntentAnalyzer& analyzer_ref, SequenceManager& seq_manager_ref)
    : intent_analyzer(analyzer_ref),
      sequence_manager(seq_manager_ref),
      time_pattern_history_limit(50)
{
    LOG_DEFAULT(LogLevel::INFO, "PredictionEngine: Initialized.");
    // Varsayılan durum düğümlerini başlat (niyet şablonlarından alabiliriz)
    // Şimdilik sadece birkaç varsayılan niyet ekleyelim
    state_graph_nodes[UserIntent::Undefined] = StateNode(UserIntent::Undefined);
    state_graph_nodes[UserIntent::Question] = StateNode(UserIntent::Question);
    state_graph_nodes[UserIntent::Command] = StateNode(UserIntent::Command);

    // load_state_graph(); // Eğer bir dosya sistemi üzerinden yükleme yapılacaksa
}

UserIntent PredictionEngine::predict_next_intent(UserIntent previous_intent, const DynamicSequence& current_sequence) const {
    if (state_graph_nodes.count(previous_intent) == 0) {
        return UserIntent::Undefined; // Önceki niyet bilinmiyorsa
    }

    const StateNode& node = state_graph_nodes.at(previous_intent);
    UserIntent predicted = UserIntent::Undefined;
    float highest_prob = 0.0f;

    // Geçiş olasılıklarına göre tahmin yap
    for (const auto& pair : node.transition_probabilities) {
        if (pair.second > highest_prob) {
            highest_prob = pair.second;
            predicted = pair.first;
        }
    }

    LOG_DEFAULT(LogLevel::DEBUG, "PredictionEngine: Niyet '" << CerebrumLux::to_string(previous_intent) << "' sonrası için tahmin edilen niyet: '" << CerebrumLux::to_string(predicted) << "' (Olasılık: " << highest_prob << ")");
    return predicted;
}

void PredictionEngine::update_state_graph(UserIntent previous_intent, UserIntent current_intent, const DynamicSequence& sequence) {
    // Düğüm yoksa oluştur
    if (state_graph_nodes.count(previous_intent) == 0) {
        state_graph_nodes[previous_intent] = StateNode(previous_intent);
    }
    if (state_graph_nodes.count(current_intent) == 0) {
        state_graph_nodes[current_intent] = StateNode(current_intent);
    }

    // Kenarı bul veya oluştur ve ağırlığını güncelle
    StateEdge* edge = find_or_create_edge(previous_intent, current_intent);
    if (edge) {
        edge->weight += 0.1f; // Geçiş ağırlığını artır (basit model)
        edge->last_transition_time_us = get_current_timestamp_us();

        // Olasılıkları yeniden normalleştir (basit bir yaklaşım)
        float total_weight = 0.0f;
        for (const auto& pair : state_graph_nodes[previous_intent].transition_probabilities) {
            total_weight += pair.second;
        }
        if (total_weight > 0) {
            for (auto& pair : state_graph_nodes[previous_intent].transition_probabilities) {
                pair.second /= total_weight;
            }
        }
    }
    LOG_DEFAULT(LogLevel::DEBUG, "PredictionEngine: Durum grafiği güncellendi: '" << CerebrumLux::to_string(previous_intent) << "' -> '" << CerebrumLux::to_string(current_intent) << "'");
}

float PredictionEngine::query_intent_probability(UserIntent target_intent, const DynamicSequence& current_sequence) const {
    // Belirli bir niyetin olma olasılığını sorgula
    // Mevcut durum ve tarihçeye göre daha karmaşık bir model olabilir
    return 0.5f; // Placeholder
}

void PredictionEngine::learn_time_patterns(const std::deque<AtomicSignal>& signal_buffer, UserIntent current_intent) {
    if (signal_buffer.size() < 2) return;

    long long first_time = signal_buffer.front().timestamp_us;
    long long last_time = signal_buffer.back().timestamp_us;
    long long duration = last_time - first_time;

    time_pattern_history[current_intent].push_back(duration);
    if (time_pattern_history[current_intent].size() > time_pattern_history_limit) {
        time_pattern_history[current_intent].pop_front();
    }
    LOG_DEFAULT(LogLevel::TRACE, "PredictionEngine: Niyet '" << CerebrumLux::to_string(current_intent) << "' için zaman deseni öğrenildi: " << duration << " us.");
}

StateEdge* PredictionEngine::find_or_create_edge(UserIntent from_intent, UserIntent to_intent) {
    std::pair<UserIntent, UserIntent> key = {from_intent, to_intent};
    auto it = state_graph_edges.find(key);
    if (it != state_graph_edges.end()) {
        return &(it->second);
    } else {
        // Yeni bir kenar oluştur
        state_graph_edges[key] = StateEdge(from_intent, to_intent);
        state_graph_nodes[from_intent].transition_probabilities[to_intent] = state_graph_edges[key].weight; // Düğümde de referans
        LOG_DEFAULT(LogLevel::DEBUG, "PredictionEngine: Yeni durum kenarı oluşturuldu: " << CerebrumLux::to_string(from_intent) << " -> " << CerebrumLux::to_string(to_intent));
        return &(state_graph_edges[key]);
    }
}

} // namespace CerebrumLux