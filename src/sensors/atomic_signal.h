#ifndef CEREBRUM_LUX_ATOMIC_SIGNAL_H
#define CEREBRUM_LUX_ATOMIC_SIGNAL_H

#include <chrono> // For timestamp_us
#include <string> // For wchar_t
#include "../core/enums.h" // Enum'lar için

// *** AtomicSignal: Kendi ozel 'bilgi atomumuz' ***
struct AtomicSignal {
    long long timestamp_us;           
    SensorType sensor_type;           
    
    // Klavye olaylari icin
    unsigned int virtual_key_code;    
    wchar_t character;                
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

    // Genel baglam
    unsigned short active_app_id_hash; 

    AtomicSignal(); // Constructor bildirimi
};

#endif // CEREBRUM_LUX_ATOMIC_SIGNAL_H