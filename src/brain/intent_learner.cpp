#include "intent_learner.h" // Kendi başlık dosyasını dahil et
#include "../core/utils.h"       // LOG_MESSAGE, intent_to_string için
#include "../data_models/dynamic_sequence.h" // DynamicSequence için
#include "../sensors/atomic_signal.h" // AtomicSignal için
#include "intent_analyzer.h"     // IntentAnalyzer için
#include "autoencoder.h"         // CryptofigAutoencoder::LATENT_DIM için (sadece LOG_MESSAGE içinde boyut kontrolü için)
#include <numeric>   // std::accumulate için
#include <cmath>     // std::sqrt, std::log10, std::fabs için
#include <algorithm> // std::min/max için
#include <iostream>  // std::wcerr için

// === IntentLearner Implementasyonlari ===

IntentLearner::IntentLearner(IntentAnalyzer& analyzer_ref) : analyzer(analyzer_ref) {
    for (int i = static_cast<int>(SensorType::Keyboard); i < static_cast<int>(SensorType::Count); ++i) {
        sensor_performance_contribution[static_cast<SensorType>(i)] = 0.0f;
        sensor_feedback_count[static_cast<SensorType>(i)] = 0;
    }
}

void IntentLearner::process_feedback(const DynamicSequence& sequence, UserIntent predicted_intent, 
                                    const std::deque<AtomicSignal>& recent_signals) {
    
    std::map<UserIntent, float> potential_feedbacks = evaluate_implicit_feedback(recent_signals);
    
    float feedback_strength = 0.0f;
    UserIntent intent_to_adjust = predicted_intent; 

    if (predicted_intent == UserIntent::Unknown) {
        float max_potential_feedback = -100.0f; 
        UserIntent best_potential_known_intent = UserIntent::Unknown;

        for (const auto& pair : potential_feedbacks) {
            if (pair.first != UserIntent::Unknown && pair.first != UserIntent::None && pair.second > max_potential_feedback) { // None'ı da hariç tut
                max_potential_feedback = pair.second;
                best_potential_known_intent = pair.first;
            }
        }
        
        if (best_potential_known_intent != UserIntent::Unknown && max_potential_feedback > 0.0f) {
            feedback_strength = max_potential_feedback;
            intent_to_adjust = best_potential_known_intent;
 
            LOG_MESSAGE(LogLevel::INFO, std::wcout, L"[AI-Ogrenen] 'Bilinmiyor' olarak tahmin edildi, ancak güçlü potansiyel niyet '" << intent_to_string(best_potential_known_intent) << L"' (geri bildirim: " << std::fixed << std::setprecision(2) << feedback_strength << L") bulundu. Bu niyet için ayar yapılıyor.\n");
        } else {
            feedback_strength = -0.5f; 
            intent_to_adjust = UserIntent::Unknown;

            LOG_MESSAGE(LogLevel::INFO, std::wcout, L"[AI-Ogrenen] Niyet 'Bilinmiyor' için hesaplanan geri bildirim gucu: " << std::fixed << std::setprecision(2) << feedback_strength << L". (Net potansiyel niyet bulunamadı.)\n");
        }
    } else {
        if (potential_feedbacks.count(predicted_intent)) {
            feedback_strength = potential_feedbacks[predicted_intent];

            LOG_MESSAGE(LogLevel::INFO, std::wcout, L"[AI-Ogrenen] Niyet '" << intent_to_string(predicted_intent) << L"' için hesaplanan geri bildirim gucu: " << std::fixed << std::setprecision(2) << feedback_strength << L"\n");
        } else {
            feedback_strength = 0.0f; 
        }
    }

    if (feedback_strength != 0.0f) { 
        adjust_template(intent_to_adjust, sequence, feedback_strength); 
        
        if (implicit_feedback_history[intent_to_adjust].size() >= feedback_history_size) {
            implicit_feedback_history[intent_to_adjust].pop_front();
        }
        implicit_feedback_history[intent_to_adjust].push_back(feedback_strength);
        
        for (const auto& sig : recent_signals) {
            sensor_performance_contribution[sig.sensor_type] += feedback_strength; 
            sensor_feedback_count[sig.sensor_type]++;
        }

        evaluate_and_meta_adjust(); 
    }
}

void IntentLearner::process_explicit_feedback(UserIntent predicted_intent, AIAction action, bool approved, const DynamicSequence& sequence) {
    float feedback_strength = approved ? 1.5f : -1.5f; 
    adjust_template(predicted_intent, sequence, feedback_strength); 
    
    float action_score_change = approved ? 1.0f : -1.0f; 
    adjust_action_score(predicted_intent, action, action_score_change);

    if (explicit_feedback_history[predicted_intent].size() >= feedback_history_size) {
        explicit_feedback_history[predicted_intent].pop_front();
    }
    explicit_feedback_history[predicted_intent].push_back(feedback_strength);

    evaluate_and_meta_adjust(); 
}

std::map<UserIntent, float> IntentLearner::evaluate_implicit_feedback(const std::deque<AtomicSignal>& recent_signals) {
    LOG_MESSAGE(LogLevel::DEBUG, std::wcerr, L"evaluate_implicit_feedback: Basladi. Recent_signals boyutu: " << recent_signals.size() << L"\n");
    std::map<UserIntent, float> potential_feedbacks;

    // Tüm niyetler için başlangıç puanlarını ekle
    for (int i = static_cast<int>(UserIntent::None); i < static_cast<int>(UserIntent::Count); ++i) {
        potential_feedbacks[static_cast<UserIntent>(i)] = 0.0f;
    }

    if (recent_signals.empty()) {
        return potential_feedbacks;
    }

    std::vector<long long> keystroke_intervals_raw; 
    int kbd_alphanumeric_count = 0;
    int kbd_backspace_count = 0;
    int kbd_control_modifier_count = 0;
    long long kbd_last_press_time = 0;
    int total_keyboard_press_events_in_buffer = 0; 

    long long mouse_total_movement_raw = 0; 
    int mouse_move_sample_count = 0;
    int mouse_click_count = 0;
    int mouse_total_events_in_buffer = 0; 
    int last_mouse_x_for_feedback = -1;
    int last_mouse_y_for_feedback = -1;

    int display_brightness_sum_raw = 0; 
    int display_sample_count = 0;
    
    int battery_change_sum_raw = 0; 
    int battery_sample_count = 0;
    unsigned char battery_last_percentage = 0;
    bool current_charging_for_feedback = false;
    
    long long network_bandwidth_sum_raw = 0; 
    int network_sample_count = 0;
    bool current_network_active_for_feedback = false;


    for (size_t i = 0; i < recent_signals.size(); ++i) {
        const AtomicSignal& sig = recent_signals[i];

        LOG_MESSAGE(LogLevel::TRACE, std::wcerr, L"evaluate_implicit_feedback: Sinyal isleniyor (indeks " << i << L", tip: " << static_cast<int>(sig.sensor_type) << L").\n");

        if (sig.sensor_type == SensorType::Keyboard) {
            if (sig.event_type == KeyEventType::Press) {
                total_keyboard_press_events_in_buffer++; 
                if (kbd_last_press_time != 0) {
                    keystroke_intervals_raw.push_back(sig.timestamp_us - kbd_last_press_time);
                }
                kbd_last_press_time = sig.timestamp_us;

                if (sig.key_type == KeyType::Alphanumeric) { 
                    kbd_alphanumeric_count++;
                } else if (sig.key_type == KeyType::Control || sig.key_type == KeyType::Modifier) { 
                    kbd_control_modifier_count++;
                } else if (sig.key_type == KeyType::Backspace) {
                    kbd_backspace_count++;
                }
            }
        } else if (sig.sensor_type == SensorType::Mouse) {
            LOG_MESSAGE(LogLevel::TRACE, std::wcerr, L"evaluate_implicit_feedback: Fare sinyali isleniyor.\n");
            mouse_total_events_in_buffer++;
            if (sig.mouse_event_type == 0) { 
                if (last_mouse_x_for_feedback != -1 && last_mouse_y_for_feedback != -1) { 
                    mouse_total_movement_raw += (std::abs(sig.mouse_x - last_mouse_x_for_feedback) + std::abs(sig.mouse_y - last_mouse_y_for_feedback));
                }
                mouse_move_sample_count++;
            } else if (sig.mouse_event_type == 1) { 
                mouse_click_count++;
            }
            last_mouse_x_for_feedback = sig.mouse_x; 
            last_mouse_y_for_feedback = sig.mouse_y; 
            // mouse_signals_in_buffer_debug++; // Debug sadece loglama için
        } else if (sig.sensor_type == SensorType::Display) {
            display_brightness_sum_raw += sig.display_brightness;
            display_sample_count++;
        } else if (sig.sensor_type == SensorType::Battery) {
            if (battery_sample_count > 0) { battery_change_sum_raw += std::abs(sig.battery_percentage - battery_last_percentage); }
            battery_last_percentage = sig.battery_percentage;
            current_charging_for_feedback = sig.battery_charging;
            battery_sample_count++;
        } else if (sig.sensor_type == SensorType::Network) {
            network_bandwidth_sum_raw += sig.network_bandwidth_estimate;
            current_network_active_for_feedback = sig.network_active;
            network_sample_count++;
        }
    } 
    LOG_MESSAGE(LogLevel::DEBUG, std::wcerr, L"evaluate_implicit_feedback: Sinyal işleme döngüsü bitti.\n");

    const float MAX_INTERVAL_LOG_BASE_MS = 10000.0f; 
    const float MAX_MOUSE_MOVEMENT_FOR_NORM = 500.0f; 
    const float MAX_NETWORK_BANDWIDTH_FOR_NORM = 15000.0f; 

    float avg_keystroke_interval_ms = 0.0f;
    float keystroke_variability_ms = 0.0f;

    float normalized_avg_keystroke_interval = 0.0f;
    float normalized_keystroke_variability = 0.0f; 
    float alphanumeric_ratio = 0.0f;
    float backspace_ratio = 0.0f;
    float control_key_frequency = 0.0f;

    float normalized_mouse_movement_intensity = 0.0f; 
    float mouse_click_frequency = 0.0f;

    float avg_brightness_norm = 0.0f;
    float battery_status_change_norm = 0.0f;
    float network_activity_level_norm = 0.0f;


    if (!keystroke_intervals_raw.empty()) {
        long long sum_intervals = std::accumulate(keystroke_intervals_raw.begin(), keystroke_intervals_raw.end(), 0LL);
        avg_keystroke_interval_ms = static_cast<float>(sum_intervals) / keystroke_intervals_raw.size() / 1000.0f; 
        if (avg_keystroke_interval_ms > 0) {
            normalized_avg_keystroke_interval = std::min(1.0f, static_cast<float>(std::log10(avg_keystroke_interval_ms + 1)) / std::log10(MAX_INTERVAL_LOG_BASE_MS + 1));
        }
        
        float sum_sq_diff_ms = 0.0f;
        for (long long interval_us : keystroke_intervals_raw) {
            float interval_ms = static_cast<float>(interval_us) / 1000.0f;
            sum_sq_diff_ms += (interval_ms - avg_keystroke_interval_ms) * (interval_ms - avg_keystroke_interval_ms);
        }
        keystroke_variability_ms = static_cast<float>(std::sqrt(static_cast<double>(sum_sq_diff_ms) / keystroke_intervals_raw.size()));
        if (keystroke_variability_ms > 0) {
            normalized_keystroke_variability = std::min(1.0f, static_cast<float>(std::log10(keystroke_variability_ms + 1)) / std::log10(MAX_INTERVAL_LOG_BASE_MS + 1));
        }
    }

    if (total_keyboard_press_events_in_buffer > 0) {
        alphanumeric_ratio = static_cast<float>(kbd_alphanumeric_count) / total_keyboard_press_events_in_buffer;
        backspace_ratio = static_cast<float>(kbd_backspace_count) / total_keyboard_press_events_in_buffer;
        control_key_frequency = static_cast<float>(kbd_control_modifier_count) / total_keyboard_press_events_in_buffer;
    }

    float raw_mouse_movement_intensity = (mouse_move_sample_count > 0) ? static_cast<float>(mouse_total_movement_raw) / mouse_move_sample_count : 0.0f;
    normalized_mouse_movement_intensity = std::min(1.0f, raw_mouse_movement_intensity / MAX_MOUSE_MOVEMENT_FOR_NORM);
    mouse_click_frequency = (mouse_total_events_in_buffer > 0) ? static_cast<float>(mouse_click_count) / mouse_total_events_in_buffer : 0.0f;

    avg_brightness_norm = (display_sample_count > 0) ? static_cast<float>(display_brightness_sum_raw) / display_sample_count / 255.0f : 0.0f;
    
    battery_status_change_norm = (battery_sample_count > 1) ? static_cast<float>(battery_change_sum_raw) / (battery_sample_count - 1) / 100.0f : 0.0f;

    float raw_network_bandwidth_avg = (network_sample_count > 0) ? static_cast<float>(network_bandwidth_sum_raw) / network_sample_count : 0.0f;
    network_activity_level_norm = std::min(1.0f, raw_network_bandwidth_avg / MAX_NETWORK_BANDWIDTH_FOR_NORM);


    // ******************************************************************************
    // YENİ: Genişletilmiş Niyetlere Göre Implicit Feedback Puanlaması
    // ******************************************************************************

    // FastTyping Koşulları
    if (alphanumeric_ratio > 0.85f && normalized_avg_keystroke_interval < 0.70f && normalized_keystroke_variability < 0.70f && normalized_mouse_movement_intensity < 0.15f && network_activity_level_norm < 0.25f) {
        potential_feedbacks[UserIntent::FastTyping] += 1.0f; 
    } else if (alphanumeric_ratio > 0.70f && normalized_avg_keystroke_interval < 0.80f && normalized_keystroke_variability < 0.80f) {
        potential_feedbacks[UserIntent::FastTyping] += 0.6f; 
    }
    if (total_keyboard_press_events_in_buffer < 5 || (normalized_mouse_movement_intensity > 0.3f && network_activity_level_norm > 0.3f)) { 
         potential_feedbacks[UserIntent::FastTyping] = std::max(-1.0f, potential_feedbacks[UserIntent::FastTyping] - 0.5f); 
    }

    // Editing Koşulları
    if ((control_key_frequency > 0.15f || backspace_ratio > 0.05f || mouse_click_frequency > 0.15f) && normalized_avg_keystroke_interval > 0.20f && normalized_avg_keystroke_interval < 0.90f && normalized_keystroke_variability > 0.10f && normalized_keystroke_variability < 0.85f) {
        potential_feedbacks[UserIntent::Editing] += 1.0f; 
    } else if ((control_key_frequency > 0.05f || backspace_ratio > 0.01f || mouse_click_frequency > 0.08f) && normalized_avg_keystroke_interval > 0.30f && normalized_keystroke_variability > 0.05f) {
        potential_feedbacks[UserIntent::Editing] += 0.7f; 
    }
    if (alphanumeric_ratio > 0.90f || (normalized_avg_keystroke_interval < 0.10f && normalized_keystroke_variability < 0.10f)) { 
        potential_feedbacks[UserIntent::Editing] = std::max(-1.0f, potential_feedbacks[UserIntent::Editing] - 0.5f);
    }

    // IdleThinking Koşulları
    if (total_keyboard_press_events_in_buffer < 3 && mouse_total_events_in_buffer < 5 && alphanumeric_ratio < 0.70f && normalized_avg_keystroke_interval > 0.75f && normalized_mouse_movement_intensity < 0.10f && network_activity_level_norm < 0.10f) {
        potential_feedbacks[UserIntent::IdleThinking] += 1.0f; 
    } else if (total_keyboard_press_events_in_buffer == 0 && mouse_total_events_in_buffer < 2 && alphanumeric_ratio < 0.90f) { 
        potential_feedbacks[UserIntent::IdleThinking] += 1.5f; 
    } else if (total_keyboard_press_events_in_buffer < 8 && alphanumeric_ratio < 0.80f && normalized_avg_keystroke_interval > 0.60f) { 
        potential_feedbacks[UserIntent::IdleThinking] += 0.7f; 
    }
    if (alphanumeric_ratio > 0.95f || normalized_avg_keystroke_interval < 0.20f) { 
        potential_feedbacks[UserIntent::IdleThinking] = std::max(-1.0f, potential_feedbacks[UserIntent::IdleThinking] - 0.5f);
    }

    // YENİ: Programming Koşulları
    if (total_keyboard_press_events_in_buffer > 10 && control_key_frequency > 0.3f && normalized_avg_keystroke_interval < 0.6f && normalized_keystroke_variability > 0.2f) {
        potential_feedbacks[UserIntent::Programming] += 1.0f;
    } else if (control_key_frequency > 0.1f && normalized_avg_keystroke_interval < 0.8f && alphanumeric_ratio > 0.5f) {
        potential_feedbacks[UserIntent::Programming] += 0.5f;
    }

    // YENİ: Gaming Koşulları
    if (normalized_mouse_movement_intensity > 0.6f && mouse_click_frequency > 0.4f && alphanumeric_ratio < 0.3f && total_keyboard_press_events_in_buffer > 5) { // Yoğun fare, bazı klavye
        potential_feedbacks[UserIntent::Gaming] += 1.5f;
    } else if (normalized_mouse_movement_intensity > 0.3f && mouse_click_frequency > 0.2f && alphanumeric_ratio < 0.5f) {
        potential_feedbacks[UserIntent::Gaming] += 0.8f;
    }

    // YENİ: MediaConsumption Koşulları
    if (total_keyboard_press_events_in_buffer < 2 && mouse_total_events_in_buffer < 5 && network_activity_level_norm > 0.1f && avg_brightness_norm > 0.5f) {
        potential_feedbacks[UserIntent::MediaConsumption] += 1.0f;
    } else if (total_keyboard_press_events_in_buffer == 0 && mouse_total_events_in_buffer < 2 && network_activity_level_norm > 0.05f) {
        potential_feedbacks[UserIntent::MediaConsumption] += 1.2f;
    }

    // YENİ: CreativeWork Koşulları
    if (normalized_keystroke_variability > 0.5f && normalized_mouse_movement_intensity > 0.4f && alphanumeric_ratio < 0.7f) {
        potential_feedbacks[UserIntent::CreativeWork] += 1.0f;
    } else if (normalized_keystroke_variability > 0.3f && normalized_mouse_movement_intensity > 0.2f && alphanumeric_ratio < 0.8f) {
        potential_feedbacks[UserIntent::CreativeWork] += 0.6f;
    }

    // YENİ: Research Koşulları
    if (network_activity_level_norm > 0.6f && mouse_click_frequency > 0.2f && normalized_avg_keystroke_interval > 0.5f) {
        potential_feedbacks[UserIntent::Research] += 1.0f;
    } else if (network_activity_level_norm > 0.3f && mouse_click_frequency > 0.1f) {
        potential_feedbacks[UserIntent::Research] += 0.7f;
    }

    // YENİ: Communication Koşulları
    if (alphanumeric_ratio > 0.7f && normalized_avg_keystroke_interval < 0.4f && control_key_frequency < 0.1f && total_keyboard_press_events_in_buffer > 5) {
        potential_feedbacks[UserIntent::Communication] += 1.0f; // Hızlı kısa metinler
    } else if (alphanumeric_ratio > 0.5f && normalized_avg_keystroke_interval < 0.6f) {
        potential_feedbacks[UserIntent::Communication] += 0.6f;
    }


    std::wstringstream ss_feedback;
    ss_feedback << L"[AI-Ogrenen] Potansiyel geri bildirimler:\n";
    for (const auto& pair : potential_feedbacks) {
        ss_feedback << L"'" << intent_to_string(pair.first) << L"': " << std::fixed << std::setprecision(2) << pair.second << L"\n";
    }
    LOG_MESSAGE(LogLevel::INFO, std::wcout, ss_feedback.str());

    LOG_MESSAGE(LogLevel::DEBUG, std::wcout, L"Metrikler(Norm): KbdAvg:" << normalized_avg_keystroke_interval
               << L" KbdVar:" << normalized_keystroke_variability
               << L" Alpha:" << alphanumeric_ratio
               << L" Control:" << control_key_frequency
               << L" Backspace:" << backspace_ratio
               << L" MouseMove:" << normalized_mouse_movement_intensity
               << L" MouseClick:" << mouse_click_frequency
               << L" Display:" << avg_brightness_norm
               << L" BatteryChange:" << battery_status_change_norm
               << L" Network:" << network_activity_level_norm
               << L" (Buffer Kbd Press:" << total_keyboard_press_events_in_buffer << L" Mouse All:" << mouse_total_events_in_buffer << L")\n");
    LOG_MESSAGE(LogLevel::DEBUG, std::wcerr, L"evaluate_implicit_feedback: Bitti. Geri bildirimler döndürülüyor.\n"); 
    return potential_feedbacks;
}