#ifndef CEREBRUM_LUX_ATOMIC_SIGNAL_H
#define CEREBRUM_LUX_ATOMIC_SIGNAL_H

#include <chrono> // For timestamp_us
#include <string> // For wchar_t
#include "../core/enums.h" // Enum'lar için
#include "../core/utils.h" // For convert_wstring_to_string (if needed elsewhere)

// *** AtomicSignal: Kendi ozel 'bilgi atomumuz' ***
struct AtomicSignal {
    long long timestamp_us;           
    SensorType sensor_type;           
    
    // Klavye olaylari icin
    unsigned int virtual_key_code;    
    char character;                
    KeyType key_type;                 
    KeyEventType event_type;          
    unsigned char pressure_estimate;  

    // Fare olaylari icin (ornek)
    int mouse_x;
    int mouse_y;
    unsigned char mouse_button_state; 
    unsigned char mouse_event_type;   

    // Ekran olaylari icin (ornek)
    unsigned char display_brightness; 
    bool display_on;

    // Batarya olaylari icin (ornek)
    unsigned char battery_percentage; 
    bool battery_charging;

    // Ag olaylari icin (ornek)
    bool network_active;
    unsigned short network_bandwidth_estimate; 

    // Mikrofon olayları için EKLENDİ
    float audio_level_db;              // Ses seviyesi (dB)
    float audio_frequency_hz;          // Baskın ses frekansı (Hz)
    bool speech_detected;              // Konuşma algılandı mı?
    unsigned short audio_environment_hash; // Ses ortamının özeti (örn: sessiz, gürültülü, müzik)

    // Kamera olayları için EKLENDİ
    float ambient_light_lux;           // Ortam ışık seviyesi (Lux)
    bool face_detected;                // Yüz algılandı mı?
    bool motion_detected;              // Hareket algılandı mı?
    unsigned short object_count;       // Algılanan nesne sayısı
    unsigned short emotion_hash;       // Algılanan duygunun özeti

    // Genel baglam
    unsigned short active_app_id_hash; 

    AtomicSignal(); // Constructor bildirimi
};

#endif // CEREBRUM_LUX_ATOMIC_SIGNAL_H