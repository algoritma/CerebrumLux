#include "atomic_signal.h" // Kendi başlık dosyasını dahil et
#include "../core/enums.h" // Gerekli enum'lar için
#include <chrono> // std::chrono için
#include "../core/logger.h"

// === AtomicSignal Implementasyonu (Constructor) ===
AtomicSignal::AtomicSignal() :
    timestamp_us(0),
    sensor_type(SensorType::Keyboard),
    virtual_key_code(0),
    character('\0'),
    key_type(KeyType::Other), 
    event_type(KeyEventType::Press),
    pressure_estimate(0),
    mouse_x(0), mouse_y(0), mouse_button_state(0), mouse_event_type(0),
    display_brightness(0), display_on(false),
    battery_percentage(0), battery_charging(false),
    network_active(false), network_bandwidth_estimate(0),
    // Mikrofon olayları için yeni alanlar EKLENDİ
    audio_level_db(0.0f),
    audio_frequency_hz(0.0f),
    speech_detected(false),
    audio_environment_hash(0),
    // Kamera olayları için yeni alanlar EKLENDİ
    ambient_light_lux(0.0f),
    face_detected(false),
    motion_detected(false),
    object_count(0),
    emotion_hash(0),
    active_app_id_hash(0) {}