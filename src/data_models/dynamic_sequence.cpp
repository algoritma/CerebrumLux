#include "dynamic_sequence.h" // Kendi başlık dosyasını dahil et
#include "../sensors/atomic_signal.h" // AtomicSignal için
#include "../brain/autoencoder.h" // CryptofigAutoencoder için
#include "../core/utils.h"       // LOG_MESSAGE için
#include <numeric>   // std::accumulate için
#include <cmath>     // std::sqrt, std::log10, std::fabs için
#include <algorithm> // std::min, std::max için
#include <iostream>  // std::wcerr için EKLEDİM

// === DynamicSequence Implementasyonlari ===

DynamicSequence::DynamicSequence() : 
    last_updated_us(0), avg_keystroke_interval(0.0f),
    keystroke_variability(0.0f), alphanumeric_ratio(0.0f), control_key_frequency(0.0f),
    mouse_movement_intensity(0.0f), mouse_click_frequency(0.0f), avg_brightness(0.0f),
    battery_status_change(0.0f), network_activity_level(0.0f),
    current_app_hash(0), 
    current_battery_percentage(0), 
    current_battery_charging(false),
    current_display_on(false), 
    current_network_active(false) 
    {}


// update_from_signals fonksiyonu CryptofigAutoencoder referansı alacak şekilde güncellendi
void DynamicSequence::update_from_signals(const std::deque<AtomicSignal>& signal_buffer, long long current_time_us, unsigned short app_hash, CryptofigAutoencoder& autoencoder) {
    LOG_MESSAGE(LogLevel::DEBUG, std::wcerr, L"DynamicSequence::update_from_signals: Basladi. Buffer boyutu: " << signal_buffer.size() << L"\n");
    last_updated_us = current_time_us;
    current_app_hash = app_hash; 

    if (signal_buffer.empty()) {
        LOG_MESSAGE(LogLevel::DEBUG, std::wcerr, L"DynamicSequence::update_from_signals: Buffer boş, sıfırlanıyor.\n");
        statistical_features_vector.clear();
        latent_cryptofig_vector.clear();
        return;
    }

    std::vector<long long> intervals;
    int alphanumeric_count = 0;
    int control_modifier_count = 0; 
    int total_keyboard_press_events = 0;
    long long last_press_timestamp = 0; 

    long long total_mouse_movement = 0;
    int mouse_move_count = 0;
    int mouse_click_count = 0;
    int mouse_total_events_in_buffer = 0; 
    int last_mouse_x_in_buffer = -1; 
    int last_mouse_y_in_buffer = -1; 
    LOG_MESSAGE(LogLevel::DEBUG, std::wcerr, L"DynamicSequence::update_from_signals: Fare metrikleri başlangıç değerleri ayarlandı.\n"); 

    int total_brightness_sum = 0; 
    int brightness_sample_count = 0;

    int total_battery_change_sum = 0; 
    int battery_sample_count = 0;
    unsigned char last_battery_percentage_val = 0; 

    long long total_bandwidth_sum = 0; 
    int network_sample_count = 0;


    for (size_t i = 0; i < signal_buffer.size(); ++i) {
        const AtomicSignal& current_sig = signal_buffer[i];

        LOG_MESSAGE(LogLevel::TRACE, std::wcerr, L"DynamicSequence::update_from_signals: Sinyal isleniyor (indeks " << i << L", tip: " << static_cast<int>(current_sig.sensor_type) << L").\n");

        if (current_sig.sensor_type == SensorType::Keyboard) {
            if (current_sig.event_type == KeyEventType::Press) {
                total_keyboard_press_events++;
                
                if (last_press_timestamp != 0) { 
                    intervals.push_back(current_sig.timestamp_us - last_press_timestamp);
                }
                last_press_timestamp = current_sig.timestamp_us; 
                
                if (current_sig.key_type == KeyType::Alphanumeric) { 
                    alphanumeric_count++;
                } else if (current_sig.key_type == KeyType::Control || current_sig.key_type == KeyType::Modifier) { 
                    control_modifier_count++;
                }
            }
        } else if (current_sig.sensor_type == SensorType::Mouse) {
            LOG_MESSAGE(LogLevel::TRACE, std::wcerr, L"DynamicSequence::update_from_signals: Fare sinyali isleniyor.\n");
            mouse_total_events_in_buffer++; 
            if (current_sig.mouse_event_type == 0) { 
                LOG_MESSAGE(LogLevel::TRACE, std::wcerr, L"DynamicSequence::update_from_signals: Fare hareket sinyali (x=" << current_sig.mouse_x << L", y=" << current_sig.mouse_y << L").\n");
                if (last_mouse_x_in_buffer != -1 && last_mouse_y_in_buffer != -1) { 
                    total_mouse_movement += (std::abs(current_sig.mouse_x - last_mouse_x_in_buffer) + std::abs(current_sig.mouse_y - last_mouse_y_in_buffer));
                    LOG_MESSAGE(LogLevel::TRACE, std::wcerr, L"DynamicSequence::update_from_signals: Toplam fare hareketi güncellendi: " << total_mouse_movement << L".\n");
                }
                mouse_move_count++;
            } else if (current_sig.mouse_event_type == 1) { 
                LOG_MESSAGE(LogLevel::TRACE, std::wcerr, L"DynamicSequence::update_from_signals: Fare tiklama sinyali (x=" << current_sig.mouse_x << L", y=" << current_sig.mouse_y << L", button=" << (int)current_sig.mouse_button_state << L").\n");
                mouse_click_count++;
            }
            last_mouse_x_in_buffer = current_sig.mouse_x;
            last_mouse_y_in_buffer = current_sig.mouse_y; 
            LOG_MESSAGE(LogLevel::TRACE, std::wcerr, L"DynamicSequence::update_from_signals: last_mouse_x_in_buffer ve last_mouse_y_in_buffer güncellendi.\n");            
        } else if (current_sig.sensor_type == SensorType::Display) {
            total_brightness_sum += current_sig.display_brightness;
            brightness_sample_count++;
            this->current_display_on = current_sig.display_on; // display_on bilgisini kaydet
        } else if (current_sig.sensor_type == SensorType::Battery) {
            if (battery_sample_count > 0) { 
                total_battery_change_sum += std::abs(current_sig.battery_percentage - last_battery_percentage_val);
            }
            last_battery_percentage_val = current_sig.battery_percentage; 
            battery_sample_count++;
            this->current_battery_percentage = current_sig.battery_percentage;
            this->current_battery_charging = current_sig.battery_charging;

        } else if (current_sig.sensor_type == SensorType::Network) {
            total_bandwidth_sum += current_sig.network_bandwidth_estimate;
            network_sample_count++;
            this->current_network_active = current_sig.network_active; // network_active bilgisini kaydet
        }
    } 
    LOG_MESSAGE(LogLevel::DEBUG, std::wcerr, L"DynamicSequence::update_from_signals: Sinyal işleme döngüsü bitti.\n");

    if (!intervals.empty()) {
        long long sum_intervals = std::accumulate(intervals.begin(), intervals.end(), 0LL);
        avg_keystroke_interval = static_cast<float>(sum_intervals) / intervals.size();
        
        long long sum_sq_diff = 0;
        for (long long interval : intervals) {
            sum_sq_diff += (interval - avg_keystroke_interval) * (interval - avg_keystroke_interval);
        }
        keystroke_variability = static_cast<float>(std::sqrt(static_cast<double>(sum_sq_diff) / intervals.size()));
    } else {
        avg_keystroke_interval = 0.0f;
        keystroke_variability = 0.0f;
    }

    if (total_keyboard_press_events > 0) {
        alphanumeric_ratio = static_cast<float>(alphanumeric_count) / total_keyboard_press_events;
        control_key_frequency = static_cast<float>(control_modifier_count) / total_keyboard_press_events;
    } else {
        alphanumeric_ratio = 0.0f;
        control_key_frequency = 0.0f;
    }

    mouse_movement_intensity = (mouse_move_count > 0) ? static_cast<float>(total_mouse_movement) / mouse_move_count : 0.0f;
    mouse_click_frequency = (total_keyboard_press_events + mouse_move_count + mouse_click_count > 0) ? static_cast<float>(mouse_click_count) / (total_keyboard_press_events + mouse_move_count + mouse_click_count) : 0.0f; 

    avg_brightness = (brightness_sample_count > 0) ? static_cast<float>(total_brightness_sum) / brightness_sample_count : 0.0f;

    battery_status_change = (battery_sample_count > 1) ? static_cast<float>(total_battery_change_sum) / (battery_sample_count - 1) : 0.0f; 
    battery_status_change /= 100.0f;

    network_activity_level = (network_sample_count > 0) ? static_cast<float>(total_bandwidth_sum) / network_sample_count : 0.0f;


    const float MAX_INTERVAL_LOG_BASE_MS = 10000.0f; 
    const float MAX_MOUSE_MOVEMENT_FOR_NORM = 500.0f; 
    const float MAX_NETWORK_BANDWIDTH_FOR_NORM = 15000.0f; 

    float normalized_avg_interval = 0.0f;
    if (avg_keystroke_interval > 0) {
        normalized_avg_interval = std::min(1.0f, static_cast<float>(std::log10(avg_keystroke_interval / 1000.0f + 1)) / std::log10(MAX_INTERVAL_LOG_BASE_MS + 1));
    }

    float normalized_variability = 0.0f;
    if (keystroke_variability > 0) {
        normalized_variability = std::min(1.0f, static_cast<float>(std::log10(keystroke_variability / 1000.0f + 1)) / std::log10(MAX_INTERVAL_LOG_BASE_MS + 1));
    }

    // statistical_features_vector dolduruluyor
    statistical_features_vector.assign({
        normalized_avg_interval,                        // Klavye 0
        normalized_variability,                         // Klavye 1
        alphanumeric_ratio,                             // Klavye 2
        control_key_frequency,                          // Klavye 3
        std::min(1.0f, mouse_movement_intensity / MAX_MOUSE_MOVEMENT_FOR_NORM), 
        mouse_click_frequency,                          // Fare 5 (zaten 0-1 arasi)
        avg_brightness / 255.0f,                        // Ekran 6 (0-255'i 0-1'e normalize et)
        battery_status_change,                          // Batarya 7 (Zaten 0-1'e normalize edildi, tekrar bölme!)
        std::min(1.0f, network_activity_level / MAX_NETWORK_BANDWIDTH_FOR_NORM), 
        static_cast<float>(current_app_hash) / 65535.0f // Uygulama 9 (hash'i 0-1'e normalize et)
    });

    // Autoencoder kullanılarak latent kriptofig üretiliyor
    latent_cryptofig_vector = autoencoder.encode(statistical_features_vector);
    // Hata durumunda Autoencoder'ın ağırlıklarını ayarla (heuristik öğrenme)
    autoencoder.adjust_weights_on_error(statistical_features_vector, 0.01f); // Öğrenme oranı örnek

    LOG_MESSAGE(LogLevel::DEBUG, std::wcerr, L"DynamicSequence::update_from_signals: Bitti. Cryptofig vektoru olusturuldu.\n");
}