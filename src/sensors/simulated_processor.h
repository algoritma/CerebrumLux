#ifndef CEREBRUM_LUX_SIMULATED_PROCESSOR_H
#define CEREBRUM_LUX_SIMULATED_PROCESSOR_H

#include "../core/enums.h"    // Enumlar için
#include "atomic_signal.h"    // AtomicSignal için
#include "signal_processor.h" // Base class için
#include "../core/utils.h"    // hash_string ve LOG için
#include <random>             // Random sayı üretimi için
#include <chrono>             // Zaman damgaları için
#include <algorithm>          // std::min/max için
#include <locale>             // std::towlower için


// *** SimulatedAtomicSignalProcessor: AtomicSignalProcessor'in simüle edilmis implementasyonu ***
class SimulatedAtomicSignalProcessor : public AtomicSignalProcessor {
private:
    long long last_key_press_time_us; 
    int current_mouse_x = 0;
    int current_mouse_y = 0;
    unsigned char current_brightness = 200;
    unsigned char current_battery = 100;
    bool current_charging = true;
    bool current_network_active = true;
    unsigned short current_network_bandwidth = 5000; 

    // Mikrofon simülasyonu için durum değişkenleri
    float current_audio_level_db = -60.0f; // dBFS (decibels relative to full scale)
    float current_audio_freq_hz = 0.0f;
    bool current_speech_detected = false;
    unsigned short current_audio_env_hash = 0; // 0: Silent, 1: Talk, 2: Music, 3: Noise

    // Kamera simülasyonu için durum değişkenleri
    float current_ambient_light_lux = 200.0f; // Typical indoor lighting
    bool current_face_detected = false;
    bool current_motion_detected = false;
    unsigned short current_object_count = 0;
    unsigned short current_emotion_hash = 0; // 0: Neutral, 1: Happy, 2: Sad, etc.

public:
    SimulatedAtomicSignalProcessor();
    AtomicSignal create_keyboard_signal(char ch); 
    
    AtomicSignal capture_next_signal() override; 
    
    bool start_capture() override;
    void stop_capture() override;
    unsigned short get_active_application_id_hash() override;

    AtomicSignal simulate_mouse_event();
    AtomicSignal simulate_display_event();
    AtomicSignal simulate_battery_event();
    AtomicSignal simulate_network_event();

    // Yeni eklendi: Mikrofon ve Kamera olaylarını simüle eden metodlar
    AtomicSignal simulate_microphone_event();
    AtomicSignal simulate_camera_event();
};

#endif // CEREBRUM_LUX_SIMULATED_PROCESSOR_H