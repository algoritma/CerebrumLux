#ifndef ATOMIC_SIGNAL_H
#define ATOMIC_SIGNAL_H

#include <string>
#include <chrono>
#include "../core/enums.h" // SensorType, KeyType, KeyEventType, MouseButtonState enum'ları için

namespace CerebrumLux { // AtomicSignal struct'ı bu namespace içine alınacak

// Tek bir atomik sensör girdisini veya dahili AI sinyalini temsil eder
struct AtomicSignal {
    std::string id; // Unique ID for the signal
    SensorType type; // Sensorun tipi (örneğin Klavye, Fare, İçsel AI, Network)
    std::string value; // Sinyalin içeriği (örneğin tuş basımı, fare koordinatları, AI kararı)
    long long timestamp_us; // Mikrosaniye cinsinden zaman damgası
    
    // Klavye sinyallerine özel
    KeyType key_type; // Alphanumeric, Control, Modifier, Function, Navigation, Other
    KeyEventType event_type; // Press, Release, Hold

    // Fare sinyallerine özel
    int mouse_x;
    int mouse_y;
    int mouse_delta_x;
    int mouse_delta_y;
    CerebrumLux::MouseButtonState mouse_button_state; // Left, Right, Middle, None

    // Ses ve Görüntü sinyallerine özel (şimdilik placeholder)
    std::vector<unsigned char> raw_audio_data;
    std::vector<unsigned char> raw_image_data;

    // AI İçsel sinyallerine özel
    std::string ai_internal_event_type;
    std::string ai_internal_event_data;
    float confidence; // AI kararının güven seviyesi

    // Ağ sinyallerine özel
    bool current_network_active; // Ağ bağlantısı var mı?
    int network_activity_level; // (0-100)
    std::string network_protocol; // HTTP, HTTPS, TCP, UDP vb.

    // Sistem olaylarına özel
    std::string system_event_type; // OS_START, APP_CRASH, LOW_BATTERY vb.
    std::string system_event_data;

    AtomicSignal()
        : id(""), type(CerebrumLux::SensorType::InternalAI), value(""), timestamp_us(0), // Namespace ile güncellendi
          key_type(CerebrumLux::KeyType::Other), event_type(CerebrumLux::KeyEventType::Press), // Namespace ile güncellendi
          mouse_x(0), mouse_y(0), mouse_delta_x(0), mouse_delta_y(0), mouse_button_state(CerebrumLux::MouseButtonState::None), // Namespace ile g├╝ncellendi
          ai_internal_event_type(""), ai_internal_event_data(""), confidence(0.0f),
          current_network_active(false), network_activity_level(0), network_protocol(""),
          system_event_type(""), system_event_data("") {}
};

} // namespace CerebrumLux

#endif // ATOMIC_SIGNAL_H