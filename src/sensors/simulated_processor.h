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
};

#endif // CEREBRUM_LUX_SIMULATED_PROCESSOR_H