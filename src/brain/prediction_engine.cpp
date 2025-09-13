#include "prediction_engine.h" // Kendi başlık dosyasını dahil et
#include "../core/utils.h"       // LOG_MESSAGE ve intent_to_string için
#include "../data_models/dynamic_sequence.h" // DynamicSequence için
#include "../data_models/sequence_manager.h" // SequenceManager için
#include "intent_analyzer.h"     // IntentAnalyzer için
#include "autoencoder.h"         // CryptofigAutoencoder::LATENT_DIM için
#include <numeric>   // std::accumulate için
#include <cmath>     // std::sqrt, std::exp için
#include <algorithm> // std::min/max için
#include <fstream>   // Dosya G/Ç için (fwprintf, fwscanf)
#include <iostream>  // std::wcerr için

// === StateNode Implementasyonu ===
StateNode::StateNode(UserIntent i) : intent(i) {}

// === StateEdge Implementasyonu ===
StateEdge::StateEdge(UserIntent from, UserIntent to) 
    : from_intent(from), to_intent(to), transition_probability(0.0f), 
      last_observed_us(0), observation_count(0) {}

// === PredictionEngine Implementasyonlari ===
PredictionEngine::PredictionEngine(IntentAnalyzer& analyzer_ref, SequenceManager& manager_ref)
    : analyzer(analyzer_ref), manager(manager_ref) {
    initialize_state_graph();
}

void PredictionEngine::initialize_state_graph() {
    // Tüm niyetleri başlangıç düğümleri olarak ekle
    for (int i = static_cast<int>(UserIntent::None); i < static_cast<int>(UserIntent::Count); ++i) {
        state_nodes.emplace_back(static_cast<UserIntent>(i));
    }
}

StateEdge* PredictionEngine::find_or_create_edge(UserIntent from_intent, UserIntent to_intent) {
    for (auto& edge : state_edges) {
        if (edge.from_intent == from_intent && edge.to_intent == to_intent) {
            return &edge;
        }
    }
    state_edges.emplace_back(from_intent, to_intent);
    return &state_edges.back();
}

void PredictionEngine::update_state_graph(UserIntent previous_intent, UserIntent current_intent, const DynamicSequence& sequence) {
    if (previous_intent == UserIntent::Unknown || current_intent == UserIntent::Unknown ||
        previous_intent == UserIntent::None || current_intent == UserIntent::None) {
        return; 
    }

    StateEdge* edge = find_or_create_edge(previous_intent, current_intent);
    if (edge) {
        if (edge->observation_count == 0) {
            edge->transition_cryptofig_delta = sequence.latent_cryptofig_vector; 
        } else {
            for (size_t i = 0; i < sequence.latent_cryptofig_vector.size(); ++i) { 
                edge->transition_cryptofig_delta[i] = 
                    (edge->transition_cryptofig_delta[i] * edge->observation_count + sequence.latent_cryptofig_vector[i]) / (edge->observation_count + 1); 
            }
        }
        edge->observation_count++;
        edge->last_observed_us = sequence.last_updated_us;
    }

    StateNode* from_node = nullptr;
    for (auto& node : state_nodes) {
        if (node.intent == previous_intent) {
            from_node = &node;
            break;
        }
    }
    if (!from_node) {
        state_nodes.emplace_back(previous_intent);
        from_node = &state_nodes.back();
    }

    if (from_node) { 
        from_node->total_outgoing_transitions++;
        for (auto& e : state_edges) {
            if (e.from_intent == previous_intent) {
                if (from_node->total_outgoing_transitions > 0) {
                    e.transition_probability = static_cast<float>(e.observation_count) / from_node->total_outgoing_transitions;
                } else {
                    e.transition_probability = 0.0f; 
                }
            }
        }
    }
}

float PredictionEngine::calculate_euclidean_distance(const std::vector<float>& vec1, const std::vector<float>& vec2) const {
    if (vec1.empty() || vec2.empty() || vec1.size() != vec2.size()) {
        LOG(LogLevel::ERR_CRITICAL, std::wcerr, L"calculate_euclidean_distance: Boyut uyuşmazlığı veya boş vektörler. Geçersiz mesafe döndürülüyor.");
        return std::numeric_limits<float>::max();
    }
    float sum_sq_diff = 0.0f;
    for (size_t i = 0; i < vec1.size(); ++i) {
        sum_sq_diff += (vec1[i] - vec2[i]) * (vec1[i] - vec2[i]);
    }
    return std::sqrt(sum_sq_diff);
}


UserIntent PredictionEngine::predict_next_intent(UserIntent previous_intent, const DynamicSequence& current_sequence) const { 
    UserIntent current_analyzed_intent = analyzer.analyze_intent(current_sequence);

    if (current_analyzed_intent == UserIntent::Unknown || previous_intent == UserIntent::Unknown ||
        current_analyzed_intent == UserIntent::None || previous_intent == UserIntent::None) {
        LOG(LogLevel::DEBUG, std::wcerr, L"predict_next_intent: Current veya Previous intent bilinmiyor/hiçbiri, doğrudan analiz döndürülüyor.\n");
        return current_analyzed_intent; 
    }

    UserIntent most_probable_future_intent = current_analyzed_intent; 
    float max_combined_score = -1.0f; 

    const float CRYPTOFIG_SIMILARITY_SCALE = 0.5f; 

    for (const auto& edge : state_edges) {
        if (edge.from_intent == current_analyzed_intent) {
            float combined_score = edge.transition_probability; 

            if (!edge.transition_cryptofig_delta.empty() && edge.transition_cryptofig_delta.size() == current_sequence.latent_cryptofig_vector.size()) {
                float distance = calculate_euclidean_distance(current_sequence.latent_cryptofig_vector, edge.transition_cryptofig_delta);
                float cryptofig_similarity = std::exp(-distance / CRYPTOFIG_SIMILARITY_SCALE); 

                float W1 = 0.7f; 
                float W2 = 0.3f; 

                combined_score = (edge.transition_probability * W1) + (cryptofig_similarity * W2);

                                LOG(LogLevel::TRACE, std::wcerr, L"Edge: " << intent_to_string(edge.from_intent) << L" -> " << intent_to_string(edge.to_intent)
                           << L", Prob: " << std::fixed << std::setprecision(4) << edge.transition_probability
                           << L", Mesafe: " << std::fixed << std::setprecision(4) << distance
                           << L", Benzerlik: " << std::fixed << std::setprecision(4) << cryptofig_similarity
                           << L", Birleşik Skor: " << std::fixed << std::setprecision(4) << combined_score);
            } else {
                 LOG(LogLevel::WARNING, std::wcerr, L"predict_next_intent: Kriptofig delta boş veya boyut uyuşmazlığı. Sadece geçiş olasılığı kullanılıyor.");
            }

            if (combined_score > max_combined_score) {
                max_combined_score = combined_score;
                most_probable_future_intent = edge.to_intent;
            }
        }
    }
    
    LOG(LogLevel::DEBUG, std::wcerr, L"predict_next_intent: En yüksek birleşik skor: " << std::fixed << std::setprecision(4) << max_combined_score);

    if (max_combined_score < 0.25f) { 
        LOG(LogLevel::DEBUG, std::wcerr, L"predict_next_intent: Birleşik skor eşiğin altında (" << std::fixed << std::setprecision(4) << max_combined_score << L" < 0.2500), mevcut analiz döndürülüyor: " << intent_to_string(current_analyzed_intent));
        return current_analyzed_intent;
    }

    LOG(LogLevel::DEBUG, std::wcerr, L"predict_next_intent: En olası gelecek niyet: " << intent_to_string(most_probable_future_intent));

    return most_probable_future_intent; 
}

float PredictionEngine::query_intent_probability(UserIntent target_intent, const DynamicSequence& current_sequence) const {
    if (analyzer.analyze_intent(current_sequence) == target_intent) {
        return 0.7f; 
    }
    return 0.3f; 
}
void PredictionEngine::learn_time_patterns(const std::deque<AtomicSignal>& signal_buffer, UserIntent current_intent) {
    (void)signal_buffer; 
    (void)current_intent;
}

void PredictionEngine::save_state_graph(const std::wstring& filename) const {
    FILE* fp = _wfopen(filename.c_str(), L"w"); 
    if (!fp) {
        LOG(LogLevel::ERR_CRITICAL, std::wcerr, L"Hata: Durum grafigi dosyasi yazilamadi: " << filename << L" (errno: " << errno << L")");
        return;
    }

    fwprintf(fp, L"%zu\n", state_nodes.size());
    for (const auto& node : state_nodes) {
        fwprintf(fp, L"%d %zu ", static_cast<int>(node.intent), node.dominant_cryptofig.size());
        for (float f : node.dominant_cryptofig) {
            fwprintf(fp, L"%.8f ", f); 
        }
        fwprintf(fp, L"%d\n", node.total_outgoing_transitions);
    }

    fwprintf(fp, L"%zu\n", state_edges.size());
    for (const auto& edge : state_edges) {
        fwprintf(fp, L"%d %d %zu ", static_cast<int>(edge.from_intent), static_cast<int>(edge.to_intent), edge.transition_cryptofig_delta.size());
        for (float f : edge.transition_cryptofig_delta) {
            fwprintf(fp, L"%.8f ", f); 
        }
        fwprintf(fp, L"%.8f %lld %d\n", edge.transition_probability, edge.last_observed_us, edge.observation_count);
    }
    fclose(fp);
    LOG(LogLevel::INFO, std::wcout, L"Durum grafigi kaydedildi: " << filename);
}

void PredictionEngine::load_state_graph(const std::wstring& filename) {
    FILE* fp = _wfopen(filename.c_str(), L"r"); 
    if (!fp) {
        LOG(LogLevel::WARNING, std::wcerr, L"Uyari: Durum grafigi dosyasi bulunamadi, bos grafik kullaniliyor: " << filename << L" (errno: " << errno << L")");
        return;
    }

    state_nodes.clear();
    state_edges.clear();

    size_t num_nodes;
    if (fwscanf(fp, L"%zu\n", &num_nodes) != 1) {
        LOG(LogLevel::ERR_CRITICAL, std::wcerr, L"Hata: Durum grafigi dosyasi formati bozuk veya bos (nodes): " << filename);
        fclose(fp);
        return;
    }
    for (size_t i = 0; i < num_nodes; ++i) {
        int intent_id_int;
        if (fwscanf(fp, L"%d", &intent_id_int) != 1) { 
            LOG(LogLevel::ERR_CRITICAL, std::wcerr, L"Hata: Durum grafigi yuklenirken intent_id okunamadi. Satir: " << i << L"\n");
            break; 
        }
        UserIntent intent = static_cast<UserIntent>(intent_id_int);

        size_t num_cryptofig_elements;
        if (fwscanf(fp, L"%zu", &num_cryptofig_elements) != 1) { 
            LOG_MESSAGE(LogLevel::ERR_CRITICAL, std::wcerr, L"Hata: Durum grafigi yuklenirken num_cryptofig_elements okunamadi. Satir: " << i << L"\n");
            break; 
        }
        std::vector<float> cryptofig(num_cryptofig_elements);
        for (size_t j = 0; j < num_cryptofig_elements; ++j) {
            if (fwscanf(fp, L"%f", &cryptofig[j]) != 1) { 
                LOG_MESSAGE(LogLevel::ERR_CRITICAL, std::wcerr, L"Hata: Durum grafigi yuklenirken cryptofig elemanı okunamadi. Satir: " << i << L", Eleman: " << j << L"\n");
                break; 
            }
        }
        
        int total_outgoing_trans; 
        if (fwscanf(fp, L"%d", &total_outgoing_trans) != 1) {
            LOG_MESSAGE(LogLevel::ERR_CRITICAL, std::wcerr, L"Hata: Durum grafigi yuklenirken total_outgoing_transitions okunamadi. Satir: " << i << L"\n");
            break;
        }
        wchar_t newline_char_node; 
        fwscanf(fp, L"%c", &newline_char_node); 

        StateNode node_temp(intent);
        node_temp.dominant_cryptofig = cryptofig;
        node_temp.total_outgoing_transitions = total_outgoing_trans;
        state_nodes.push_back(node_temp); 
    }

    size_t num_edges;
    if (fwscanf(fp, L"%zu\n", &num_edges) != 1) {
        LOG_MESSAGE(LogLevel::ERR_CRITICAL, std::wcerr, L"Hata: Durum grafigi dosyasi formati bozuk veya bos (edges): " << filename << L"\n");
        fclose(fp);
        return;
    }
    for (size_t i = 0; i < num_edges; ++i) {
        int from_intent_int, to_intent_int;
        if (fwscanf(fp, L"%d %d", &from_intent_int, &to_intent_int) != 2) { 
            LOG_MESSAGE(LogLevel::ERR_CRITICAL, std::wcerr, L"Hata: Durum grafigi yuklenirken from_intent/to_intent okunamadi. Satir: " << i << L"\n");
            break; 
        }
        UserIntent from_intent = static_cast<UserIntent>(from_intent_int);
        UserIntent to_intent = static_cast<UserIntent>(to_intent_int);

        size_t num_delta_elements;
        if (fwscanf(fp, L"%zu", &num_delta_elements) != 1) { 
            LOG_MESSAGE(LogLevel::ERR_CRITICAL, std::wcerr, L"Hata: Durum grafigi yuklenirken num_delta_elements okunamadi. Satir: " << i << L"\n");
            break; 
        }
        std::vector<float> delta(num_delta_elements);
        for (size_t j = 0; j < num_delta_elements; ++j) {
            if (fwscanf(fp, L"%f", &delta[j]) != 1) { 
                LOG_MESSAGE(LogLevel::ERR_CRITICAL, std::wcerr, L"Hata: Durum grafigi yuklenirken delta elemanı okunamadi. Satir: " << i << L", Eleman: " << j << L"\n");
                break; 
            }
        }

        float prob;
        long long last_obs_us;
        int obs_count;
        if (fwscanf(fp, L"%f %lld %d", &prob, &last_obs_us, &obs_count) != 3) {
            LOG_MESSAGE(LogLevel::ERR_CRITICAL, std::wcerr, L"Hata: Durum grafigi yuklenirken edge verileri okunamadi. Satir: " << i << L"\n");
            break; 
        } 
        wchar_t newline_char_edge; 
        fwscanf(fp, L"%c", &newline_char_edge); 
        
        state_edges.emplace_back(from_intent, to_intent);
        state_edges.back().transition_cryptofig_delta = delta;
        state_edges.back().transition_probability = prob;
        state_edges.back().last_observed_us = last_obs_us;
        state_edges.back().observation_count = obs_count;
    }
    fclose(fp);
    LOG_MESSAGE(LogLevel::INFO, std::wcout, L"Durum grafigi yuklendi: " << filename << L"\n");
}