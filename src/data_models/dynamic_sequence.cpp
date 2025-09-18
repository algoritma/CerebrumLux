#include "dynamic_sequence.h" // Kendi başlık dosyasını dahil et
#include "../sensors/atomic_signal.h" // AtomicSignal için
#include "../brain/autoencoder.h" // CryptofigAutoencoder için
#include "../core/logger.h"      // LOG makrosu için
#include "../core/utils.h"       // Diğer yardımcı fonksiyonlar için
#include <numeric>   // std::accumulate için
#include <cmath>     // std::sqrt, std::log10, std::fabs için
#include <algorithm> // std::min, std::max için
#include <iostream>  // std::cout, std::cerr için
#include <iomanip>   // std::fixed, std::setprecision için
#include <sstream>   // std::stringstream için
#include <map>       // YENİ: std::map için eklendi


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
    current_network_active(false),
    // Yeni eklenen sensör verisi alanları başlatılıyor
    avg_audio_level_db(0.0f),
    avg_audio_frequency_hz(0.0f),
    speech_detection_ratio(0.0f),
    dominant_audio_environment_hash(0),
    avg_ambient_light_lux(0.0f),
    face_detection_ratio(0.0f),
    motion_detection_ratio(0.0f),
    avg_object_count(0),
    dominant_emotion_hash(0)
    {}


// update_from_signals fonksiyonu CryptofigAutoencoder referansı alacak şekilde güncellendi
void DynamicSequence::update_from_signals(const std::deque<AtomicSignal>& signal_buffer, long long current_time_us, unsigned short app_hash, CryptofigAutoencoder& autoencoder) {
    LOG_DEFAULT(LogLevel::DEBUG, "DynamicSequence::update_from_signals: Basladi. Buffer boyutu: " << signal_buffer.size() << "\n");
    last_updated_us = current_time_us;
    current_app_hash = app_hash; 

    if (signal_buffer.empty()) {
        LOG_DEFAULT(LogLevel::DEBUG, "DynamicSequence::update_from_signals: Buffer boş, sıfırlanıyor.\n");
        statistical_features_vector.clear();
        latent_cryptofig_vector.clear();
        // Yeni eklenen alanları da sıfırla
        avg_audio_level_db = 0.0f; avg_audio_frequency_hz = 0.0f; speech_detection_ratio = 0.0f; dominant_audio_environment_hash = 0;
        avg_ambient_light_lux = 0.0f; face_detection_ratio = 0.0f; motion_detection_ratio = 0.0f; avg_object_count = 0; dominant_emotion_hash = 0;
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
    LOG_DEFAULT(LogLevel::DEBUG, "DynamicSequence::update_from_signals: Fare metrikleri başlangıç değerleri ayarlandı.\n"); 

    int total_brightness_sum = 0; 
    int brightness_sample_count = 0;

    int total_battery_change_sum = 0; 
    int battery_sample_count = 0;
    unsigned char last_battery_percentage_val = 0; 

    long long total_bandwidth_sum = 0; 
    int network_sample_count = 0;

    // Mikrofon ve Kamera için yeni toplama değişkenleri
    float total_audio_level_db_sum = 0.0f;
    float total_audio_frequency_hz_sum = 0.0f;
    int audio_sample_count = 0;
    int speech_detected_count_total = 0;
    std::map<unsigned short, int> audio_environment_counts; // dominant_audio_environment_hash için

    float total_ambient_light_lux_sum = 0.0f;
    int camera_sample_count = 0;
    int face_detected_count_total = 0;
    int motion_detected_count_total = 0;
    unsigned short total_object_count_sum = 0;
    std::map<unsigned short, int> emotion_counts; // dominant_emotion_hash için


    for (size_t i = 0; i < signal_buffer.size(); ++i) {
        const AtomicSignal& current_sig = signal_buffer[i];

        LOG_DEFAULT(LogLevel::TRACE, "DynamicSequence::update_from_signals: Sinyal isleniyor (indeks " << i << ", tip: " << static_cast<int>(current_sig.sensor_type) << ")\n");

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
            LOG_DEFAULT(LogLevel::TRACE, "DynamicSequence::update_from_signals: Fare sinyali isleniyor.\n");
            mouse_total_events_in_buffer++; 
            if (current_sig.mouse_event_type == 0) { 
                LOG_DEFAULT(LogLevel::TRACE, "DynamicSequence::update_from_signals: Fare hareket sinyali (x=" << current_sig.mouse_x << ", y=" << current_sig.mouse_y << ").\n");
                if (last_mouse_x_in_buffer != -1 && last_mouse_y_in_buffer != -1) { 
                    total_mouse_movement += (std::abs(current_sig.mouse_x - last_mouse_x_in_buffer) + std::abs(current_sig.mouse_y - last_mouse_y_in_buffer));
                    LOG_DEFAULT(LogLevel::TRACE, "DynamicSequence::update_from_signals: Toplam fare hareketi güncellendi: " << total_mouse_movement << ".\n");
                }
                mouse_move_count++;
            } else if (current_sig.mouse_event_type == 1) { 
                LOG_DEFAULT(LogLevel::TRACE, "DynamicSequence::update_from_signals: Fare tiklama sinyali (x=" << current_sig.mouse_x << ", y=" << current_sig.mouse_y << ", button=" << static_cast<int>(current_sig.mouse_button_state) << ").\n");
                mouse_click_count++;
            }
            last_mouse_x_in_buffer = current_sig.mouse_x;
            last_mouse_y_in_buffer = current_sig.mouse_y; 
            LOG_DEFAULT(LogLevel::TRACE, "DynamicSequence::update_from_signals: last_mouse_x_in_buffer ve last_mouse_y_in_buffer güncellendi.\n");            
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
        } else if (current_sig.sensor_type == SensorType::Microphone) { // YENİ: Mikrofon sinyallerini işle
            total_audio_level_db_sum += current_sig.audio_level_db;
            total_audio_frequency_hz_sum += current_sig.audio_frequency_hz;
            if (current_sig.speech_detected) {
                speech_detected_count_total++;
            }
            audio_environment_counts[current_sig.audio_environment_hash]++;
            audio_sample_count++;
        } else if (current_sig.sensor_type == SensorType::Camera) { // YENİ: Kamera sinyallerini işle
            total_ambient_light_lux_sum += current_sig.ambient_light_lux;
            if (current_sig.face_detected) {
                face_detected_count_total++;
            }
            if (current_sig.motion_detected) {
                motion_detected_count_total++;
            }
            total_object_count_sum += current_sig.object_count;
            emotion_counts[current_sig.emotion_hash]++;
            camera_sample_count++;
        }
    } 
    LOG_DEFAULT(LogLevel::DEBUG, "DynamicSequence::update_from_signals: Sinyal işleme döngüsü bitti.\n");

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

    // YENİ: Mikrofon ve Kamera metriklerinin hesaplanması
    avg_audio_level_db = (audio_sample_count > 0) ? total_audio_level_db_sum / audio_sample_count : 0.0f;
    avg_audio_frequency_hz = (audio_sample_count > 0) ? total_audio_frequency_hz_sum / audio_sample_count : 0.0f;
    speech_detection_ratio = (audio_sample_count > 0) ? static_cast<float>(speech_detected_count_total) / audio_sample_count : 0.0f;
    // En baskın ses ortamını bul
    dominant_audio_environment_hash = 0;
    int max_audio_env_count = 0;
    for (const auto& pair : audio_environment_counts) {
        if (pair.second > max_audio_env_count) {
            max_audio_env_count = pair.second;
            dominant_audio_environment_hash = pair.first;
        }
    }

    avg_ambient_light_lux = (camera_sample_count > 0) ? total_ambient_light_lux_sum / camera_sample_count : 0.0f;
    face_detection_ratio = (camera_sample_count > 0) ? static_cast<float>(face_detected_count_total) / camera_sample_count : 0.0f;
    motion_detection_ratio = (camera_sample_count > 0) ? static_cast<float>(motion_detected_count_total) / camera_sample_count : 0.0f;
    avg_object_count = (camera_sample_count > 0) ? static_cast<unsigned short>(static_cast<float>(total_object_count_sum) / camera_sample_count + 0.5f) : 0; // Yuvarlama
    // En baskın duyguyu bul
    dominant_emotion_hash = 0;
    int max_emotion_count = 0;
    for (const auto& pair : emotion_counts) {
        if (pair.second > max_emotion_count) {
            max_emotion_count = pair.second;
            dominant_emotion_hash = pair.first;
        }
    }


    const float MAX_INTERVAL_LOG_BASE_MS = 10000.0f; 
    const float MAX_MOUSE_MOVEMENT_FOR_NORM = 500.0f; 
    const float MAX_NETWORK_BANDWIDTH_FOR_NORM = 15000.0f; 
    const float MAX_AUDIO_LEVEL_DB_FOR_NORM = 90.0f; // -90dB (min) to 0dB (max)
    const float MAX_AUDIO_FREQ_HZ_FOR_NORM = 20000.0f;
    const float MAX_AMBIENT_LIGHT_LUX_FOR_NORM = 1000.0f; // İç/dış mekan için tipik bir üst limit
    const float MAX_OBJECT_COUNT_FOR_NORM = 5.0f; // Max beklenen nesne sayısı


    float normalized_avg_interval = 0.0f;
    if (avg_keystroke_interval > 0) {
        normalized_avg_interval = std::min(1.0f, static_cast<float>(std::log10(avg_keystroke_interval / 1000.0f + 1)) / std::log10(MAX_INTERVAL_LOG_BASE_MS + 1));
    }

    float normalized_variability = 0.0f;
    if (keystroke_variability > 0) {
        normalized_variability = std::min(1.0f, static_cast<float>(std::log10(keystroke_variability / 1000.0f + 1)) / std::log10(MAX_INTERVAL_LOG_BASE_MS + 1));
    }

    // YENİ: Mikrofon ve Kamera metriklerinin normalize edilmesi
    float normalized_audio_level = std::min(1.0f, (avg_audio_level_db + MAX_AUDIO_LEVEL_DB_FOR_NORM) / MAX_AUDIO_LEVEL_DB_FOR_NORM); // -90dB'yi 0'a, 0dB'yi 1'e çevir
    float normalized_audio_freq = std::min(1.0f, avg_audio_frequency_hz / MAX_AUDIO_FREQ_HZ_FOR_NORM);
    float normalized_ambient_light = std::min(1.0f, avg_ambient_light_lux / MAX_AMBIENT_LIGHT_LUX_FOR_NORM);
    float normalized_object_count = std::min(1.0f, static_cast<float>(avg_object_count) / MAX_OBJECT_COUNT_FOR_NORM);
    // Hash değerlerini 0-1 aralığına normalize et (maksimum unsigned short değeri 65535)
    float normalized_audio_env_hash = static_cast<float>(dominant_audio_environment_hash) / 65535.0f;
    float normalized_emotion_hash = static_cast<float>(dominant_emotion_hash) / 65535.0f;


    // statistical_features_vector dolduruluyor (YENİ SENSÖR VERİLERİ EKLENDİ)
    statistical_features_vector.assign({
        normalized_avg_interval,                        // Klavye 0
        normalized_variability,                         // Klavye 1
        alphanumeric_ratio,                             // Klavye 2
        control_key_frequency,                          // Klavye 3
        std::min(1.0f, mouse_movement_intensity / MAX_MOUSE_MOVEMENT_FOR_NORM), // Fare 4
        mouse_click_frequency,                          // Fare 5 (zaten 0-1 arasi)
        avg_brightness / 255.0f,                        // Ekran 6 (0-255'i 0-1'e normalize et)
        battery_status_change,                          // Batarya 7 (Zaten 0-1'e normalize edildi, tekrar bölme!)
        std::min(1.0f, network_activity_level / MAX_NETWORK_BANDWIDTH_FOR_NORM), // Ağ 8
        static_cast<float>(current_app_hash) / 65535.0f, // Uygulama 9 (hash'i 0-1'e normalize et)
        // YENİ MİKROFON ÖZELLİKLERİ (10, 11, 12, 13)
        normalized_audio_level,                         // Mikrofon 10
        normalized_audio_freq,                          // Mikrofon 11
        speech_detection_ratio,                         // Mikrofon 12
        normalized_audio_env_hash,                      // Mikrofon 13
        // YENİ KAMERA ÖZELLİKLERİ (14, 15, 16, 17)
        normalized_ambient_light,                       // Kamera 14
        face_detection_ratio,                           // Kamera 15
        motion_detection_ratio,                         // Kamera 16
        normalized_object_count,                        // Kamera 17
        normalized_emotion_hash                         // Kamera 18
    });

    // Autoencoder kullanılarak latent kriptofig üretiliyor
    if (statistical_features_vector.size() != CryptofigAutoencoder::INPUT_DIM) {
        LOG_DEFAULT(LogLevel::ERR_CRITICAL, "DynamicSequence::update_from_signals: statistical_features_vector boyutu ( " << statistical_features_vector.size() << ") CryptofigAutoencoder::INPUT_DIM (" << CryptofigAutoencoder::INPUT_DIM << ") ile uyuşmuyor! Autoencoder işlemi atlanıyor.\n");
        latent_cryptofig_vector.assign(CryptofigAutoencoder::LATENT_DIM, 0.0f); // Latent vektörü sıfırla
    } else {
        latent_cryptofig_vector = autoencoder.encode(statistical_features_vector);
        // Hata durumunda Autoencoder'ın ağırlıklarını ayarla (heuristik öğrenme)
        autoencoder.adjust_weights_on_error(statistical_features_vector, 0.01f); // Öğrenme oranı örnek
    }

    LOG_DEFAULT(LogLevel::DEBUG, "DynamicSequence::update_from_signals: Bitti. Cryptofig vektoru olusturuldu.\n");
}