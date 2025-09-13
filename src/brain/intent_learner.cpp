#include "intent_learner.h" // Kendi başlık dosyasını dahil et
#include "../core/utils.h"       // intent_to_string için
#include "../core/logger.h"      // LOG makrosu için
#include "../data_models/dynamic_sequence.h" // DynamicSequence için
#include "intent_analyzer.h"     // IntentAnalyzer için
#include "atomic_signal.h"       // AtomicSignal için
#include <numeric> // std::accumulate
#include <algorithm> // std::min, std::max
#include <cmath>     // std::abs
#include <iomanip>   // std::fixed, std::setprecision
#include <iostream>  // std::wcout, std::wcerr

// === IntentLearner Implementasyonlari ===

IntentLearner::IntentLearner(IntentAnalyzer& analyzer_ref)
    : analyzer(analyzer_ref), learning_rate(0.01f) {}

void IntentLearner::process_feedback(const DynamicSequence& sequence, UserIntent predicted_intent, const std::deque<AtomicSignal>& recent_signals) {
    // Geri bildirim gücünü hesapla (örneğin, tahmin edilen niyetin güvenine göre)
    float feedback_strength = 0.0f; // Varsayılan olarak sıfır

    // Eğer tahmin edilen niyet Unknown ise ve güçlü bir potansiyel niyet varsa
    if (predicted_intent == UserIntent::Unknown) {
        UserIntent best_potential_known_intent = UserIntent::Unknown;
        float max_potential_score = -std::numeric_limits<float>::max();

        for (const auto& tmpl : analyzer.intent_templates) { // Corrected access
            if (tmpl.id != UserIntent::Unknown) {
                float score = 0.0f;
                for (size_t i = 0; i < tmpl.weights.size(); ++i) {
                    score += tmpl.weights[i] * sequence.latent_cryptofig_vector[i];
                }
                if (score > max_potential_score) {
                    max_potential_score = score;
                    best_potential_known_intent = tmpl.id;
                }
            }
        }

        if (best_potential_known_intent != UserIntent::Unknown && max_potential_score > analyzer.confidence_threshold_for_known_intent) { // Corrected access
            LOG(LogLevel::INFO, L"[AI-Ogrenen] 'Bilinmiyor' olarak tahmin edildi, ancak güçlü potansiyel niyet '" << intent_to_string(best_potential_known_intent) << L"' (geri bildirim: " << std::fixed << std::setprecision(2) << feedback_strength << L") bulundu. Bu niyet için ayar yapılıyor.\n");
            // Bu durumda, best_potential_known_intent için ağırlıkları ayarla
            std::vector<float> current_weights = analyzer.get_intent_weights(best_potential_known_intent);
            for (size_t i = 0; i < current_weights.size(); ++i) {
                current_weights[i] += learning_rate * (sequence.latent_cryptofig_vector[i] - current_weights[i]);
                current_weights[i] = std::min(5.0f, std::max(-5.0f, current_weights[i]));
            }
            analyzer.update_template_weights(best_potential_known_intent, current_weights);
        } else {
            // Gerçekten Unknown ise, öğrenme oranını düşür veya hiçbir şey yapma
            feedback_strength = 0.0f;
            LOG(LogLevel::INFO, L"[AI-Ogrenen] Niyet 'Bilinmiyor' için hesaplanan geri bildirim gucu: " << std::fixed << std::setprecision(2) << feedback_strength << L". (Net potansiyel niyet bulunamadı.)\n");
        }
    }
    else {
        // Bilinen bir niyet tahmin edildiyse, bu niyet için ağırlıkları ayarla
        feedback_strength = 1.0f; // Pozitif geri bildirim
        LOG(LogLevel::INFO, L"[AI-Ogrenen] Niyet '" << intent_to_string(predicted_intent) << L"' için hesaplanan geri bildirim gucu: " << std::fixed << std::setprecision(2) << feedback_strength << L"\n");
        std::vector<float> current_weights = analyzer.get_intent_weights(predicted_intent);
        for (size_t i = 0; i < current_weights.size(); ++i) {
            current_weights[i] += learning_rate * (sequence.latent_cryptofig_vector[i] - current_weights[i]);
            current_weights[i] = std::min(5.0f, std::max(-5.0f, current_weights[i]));
        }
        analyzer.update_template_weights(predicted_intent, current_weights);
    }

    // Implicit geri bildirim metriklerini topla ve işle
    AbstractState current_abstract_state = infer_abstract_state(recent_signals);
    evaluate_implicit_feedback(predicted_intent, current_abstract_state);
}

void IntentLearner::evaluate_implicit_feedback(UserIntent current_intent, AbstractState current_abstract_state) {
    double implicit_feedback_score = 0.0; // Varsayılan nötr geri bildirim

    // AbstractState'e göre genel bir modifikasyon yapısı
    switch (current_abstract_state) {
        case AbstractState::PowerSaving:
            // Enerji tasarrufu modundayken farklı niyetler nasıl etkilenmeli?
            if (current_intent == UserIntent::Gaming ||
                current_intent == UserIntent::VideoEditing ||
                current_intent == UserIntent::CreativeWork) { // Yüksek performans gerektiren niyetler
                implicit_feedback_score = -0.5; // Güç tasarrufunda kötü performans -> negatif geri bildirim
                LOG(LogLevel::INFO, L"[AI-Ogrenen] Güç Tasarrufu modunda Yüksek Performans Niyeti algılandı. Negatif dolaylı geri bildirim uygulandı.");
            } else if (current_intent == UserIntent::MediaConsumption) { // Video izleme gibi, duruma göre değişebilir
                // Daha detaylı kontrol: çözünürlük düşükse nötr, yüksekse hafif negatif olabilir.
                // Şimdilik hafif negatif varsayalım, kullanıcı deneyimi tam olmayabilir.
                implicit_feedback_score = -0.1;
                LOG(LogLevel::INFO, L"[AI-Ogrenen] Güç Tasarrufu modunda Medya Tüketimi Niyeti algılandı. Hafif negatif dolaylı geri bildirim uygulandı.");
            } else if (current_intent == UserIntent::Browsing ||
                       current_intent == UserIntent::Reading ||
                       current_intent == UserIntent::GeneralInquiry) { // Düşük güç gerektiren niyetler
                implicit_feedback_score = 0.0; // Nötr veya hafif pozitif (eğer kullanıcı uzun pil ömrüne değer veriyorsa)
                LOG(LogLevel::INFO, L"[AI-Ogrenen] Güç Tasarrufu modunda Düşük Performans Niyeti algılandı. Nötr dolaylı geri bildirim uygulandı.");
            }
            break;

        case AbstractState::HighPerformance:
            // Yüksek performans modundayken niyetler için pozitif geri bildirim
            if (current_intent == UserIntent::Gaming ||
                current_intent == UserIntent::VideoEditing ||
                current_intent == UserIntent::CreativeWork) {
                implicit_feedback_score = 0.3; // Yüksek performansta iyi deneyim -> pozitif geri bildirim
            }
            break;

        case AbstractState::FaultyHardware: // Örneğin arızalı donanım durumu
             if (current_intent != UserIntent::None) { // Herhangi bir niyet için genel olarak negatif
                implicit_feedback_score = -0.7; // Donanım arızası tüm niyetleri olumsuz etkiler
             }
             break;
        // ... Diğer AbstractState'ler ve onların UserIntent'ler üzerindeki etkileri buraya eklenebilir

        default:
            // Varsayılan durum: Özel bir durum yoksa geri bildirim nötr kalır veya başka bir mantık uygulanır
            implicit_feedback_score = 0.0;
            break;
    }

    LOG(LogLevel::DEBUG, L"[AI-Ogrenen] Niyet: " << static_cast<int>(current_intent)
                         << L", Durum: " << static_cast<int>(current_abstract_state)
                         << L", Dolaylı Geri Bildirim Puanı: " << implicit_feedback_score);

    // implicit_feedback_history'yi güncelle
    implicit_feedback_history[current_intent].push_back(implicit_feedback_score);
    if (implicit_feedback_history[current_intent].size() > feedback_history_size) {
        implicit_feedback_history[current_intent].pop_front();
    }
}

AbstractState IntentLearner::infer_abstract_state(const std::deque<AtomicSignal>& recent_signals) {
    // Sinyallerden AbstractState çıkarmak için basit bir mantık
    // Daha karmaşık bir çıkarım mekanizması burada geliştirilebilir.
    // Şimdilik, en baskın AbstractState'i belirlemeye çalışalım.

    std::map<AbstractState, float> state_scores;
    state_scores[AbstractState::None] = 0.0f; // Varsayılan durum

    // Sinyal türlerine göre ağırlıklar (örnek değerler, orijinalden alınmıştır)
    const float BRIGHTNESS_WEIGHT = 0.1f;
    const float BATTERY_WEIGHT = 0.1f;
    // Diğer sensör tipleri için de benzer ağırlıklar eklenebilir

    for (const auto& sig : recent_signals) {
        switch (sig.sensor_type) {
            case SensorType::Display: {
                float brightness = sig.display_brightness;
                if (brightness < 0.3f) {
                    state_scores[AbstractState::PowerSaving] += BRIGHTNESS_WEIGHT * 0.5f;
                }
                // Yüksek parlaklık için HighPerformance durumu da eklenebilir
                else if (brightness > 0.8f) {
                    state_scores[AbstractState::HighPerformance] += BRIGHTNESS_WEIGHT * 0.3f;
                }
                break;
            }
            case SensorType::Battery: {
                float change = sig.battery_percentage; // Pil yüzdesi değişimi olarak kullanılıyor
                if (change < -0.05f) { // %5'ten fazla düşüş
                    state_scores[AbstractState::FaultyHardware] += BATTERY_WEIGHT * 0.7f;
                }
                break;
            }
            // Diğer sensör tipleri için AbstractState çıkarım mantığı buraya eklenebilir
            // Örneğin:
            // case SensorType::Temperature: {
            //     float temp = sig.value;
            //     if (temp > 80) { // Cihaz çok sıcak
            //         state_scores[AbstractState::FaultyHardware] += 0.1f;
            //     }
            //     break;
            // }
            // case SensorType::AmbientLight: {
            //     float light = sig.value;
            //     if (light < 50) { // Karanlık ortam
            //         state_scores[AbstractState::PowerSaving] += 0.05f;
            //     }
            //     break;
            // }
            // case SensorType::HeartRate: {
            //     float hr = sig.value;
            //     if (hr > 100) { // Yüksek kalp atış hızı
            //         state_scores[AbstractState::Distracted] += 0.05f; // Stres
            //     }
            //     break;
            // }
            default:
                break;
        }
    }

    // En yüksek puana sahip AbstractState'i bul
    AbstractState inferred_state = AbstractState::None;
    float max_score = 0.0f;

    for (const auto& pair : state_scores) {
        if (pair.second > max_score) {
            max_score = pair.second;
            inferred_state = pair.first;
        }
    }

    LOG(LogLevel::DEBUG, L"[AI-Ogrenen] Sinyallerden çıkarılan AbstractState: " << static_cast<int>(inferred_state) << L" (Puan: " << max_score << L")");
    return inferred_state;
}

void IntentLearner::adjust_learning_rate(float rate) {
    learning_rate = std::min(0.1f, std::max(0.001f, rate)); // Öğrenme oranını belirli sınırlar içinde tut
}

// Removed redefinition of get_learning_rate() as it's already defined in the header
// float IntentLearner::get_learning_rate() const {
//     return learning_rate;
// }