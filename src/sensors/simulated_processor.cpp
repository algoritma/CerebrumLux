#include "simulated_processor.h" // Kendi başlık dosyasını dahil et
#include "../core/utils.h"       // LOG ve hash_string için
#include <iostream>              // std::cout, std::cerr için
#include <numeric>               // std::accumulate için
#include <cmath>                 // std::abs için
#include <algorithm>             // std::min, std::max, std::towlower için
#include <chrono>                // std::chrono için
#include <locale> // std::towlower için (cerebrum_lux_core.cpp'den geldi)
#include "../core/logger.h"
#include <sstream>   // std::stringstream için

// Statik global değişkenler olarak başlatılması
static std::mt19937 s_gen([]() {
    std::random_device rd_local;
    return rd_local();
}());

static std::uniform_int_distribution<int> s_sensor_selection_distrib(0, static_cast<int>(SensorType::Count) - 1); 

SimulatedAtomicSignalProcessor::SimulatedAtomicSignalProcessor() : last_key_press_time_us(0) {}

bool SimulatedAtomicSignalProcessor::start_capture() {
    LOG_DEFAULT(LogLevel::INFO, "Simulasyon baslatildi. Tuslara basin (Q ile çikis).\n");
    last_key_press_time_us = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count();
    return true;
}

void SimulatedAtomicSignalProcessor::stop_capture() { 
    LOG_DEFAULT(LogLevel::INFO, "Simulasyon durduruldu.\n");
}

unsigned short SimulatedAtomicSignalProcessor::get_active_application_id_hash() {
    static int call_count = 0; 
        static std::mt19937 app_gen([]() { std::random_device rd_local; return rd_local(); }());  
    
    call_count++;
    if (call_count % 5 == 0) { 
        std::uniform_int_distribution<> distrib_app_id(0, 5); 
        int app_choice = distrib_app_id(app_gen); 
        if (app_choice == 0) return hash_string("Tarayici"); 
        else if (app_choice == 1) return hash_string("MetinEditoru");
        else if (app_choice == 2) return hash_string("Terminal");
        else if (app_choice == 3) return hash_string("OyunUygulamasi");
        else if (app_choice == 4) return hash_string("VideoOynatici");
        else return hash_string("IDE"); 
    }
    return hash_string("VarsayilanUygulama"); 
}

AtomicSignal SimulatedAtomicSignalProcessor::create_keyboard_signal(char ch) {
    AtomicSignal keyboard_signal;
    long long current_time_us = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count();
    keyboard_signal.timestamp_us = current_time_us; 

    keyboard_signal.active_app_id_hash = get_active_application_id_hash();
    keyboard_signal.sensor_type = SensorType::Keyboard; 
    keyboard_signal.character = ch;
    keyboard_signal.virtual_key_code = static_cast<unsigned int>(ch);
    keyboard_signal.event_type = KeyEventType::Press; 

    if (last_key_press_time_us != 0) {
        long long interval = current_time_us - last_key_press_time_us;
        interval = std::max(50000LL, std::min(1000000LL, interval)); 
        keyboard_signal.pressure_estimate = static_cast<unsigned char>(255 - (interval * 255 / 1000000LL)); 
        LOG_DEFAULT(LogLevel::DEBUG, "Klavye sinyali aralığı: " << (interval / 1000.0f) << " ms, Basınç: " << static_cast<int>(keyboard_signal.pressure_estimate) << "\n");
    } else {
        keyboard_signal.pressure_estimate = 128; 
    }
    last_key_press_time_us = current_time_us; 

    char lower_ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    if (std::isalpha(lower_ch) || std::isdigit(lower_ch)) { 
        keyboard_signal.key_type = KeyType::Alphanumeric;
    } else if (lower_ch == ' ') { 
        keyboard_signal.key_type = KeyType::Whitespace;
    } else if (lower_ch == '\r' || lower_ch == '\n') { 
        keyboard_signal.key_type = KeyType::Enter;
    } else if (lower_ch == '\b') { 
        keyboard_signal.key_type = KeyType::Backspace;
    } else {
        keyboard_signal.key_type = KeyType::Other;
    }

    return keyboard_signal;
}

AtomicSignal SimulatedAtomicSignalProcessor::capture_next_signal() {
    AtomicSignal signal; 

    std::uniform_int_distribution<> other_sensor_type_distrib(static_cast<int>(SensorType::Mouse), static_cast<int>(SensorType::Keyboard)); 
    int chosen_sensor_type_for_sim = other_sensor_type_distrib(s_gen);

    LOG_DEFAULT(LogLevel::TRACE, "SimulatedAtomicSignalProcessor::capture_next_signal: Rastgele seçilen klavye dışı sensor tipi: " << chosen_sensor_type_for_sim << ".\n");

    if (static_cast<SensorType>(chosen_sensor_type_for_sim) == SensorType::Mouse) { 
        signal = simulate_mouse_event();
    } else if (static_cast<SensorType>(chosen_sensor_type_for_sim) == SensorType::Display) { 
        signal = simulate_display_event();
    } else if (static_cast<SensorType>(chosen_sensor_type_for_sim) == SensorType::Battery) { 
        signal = simulate_battery_event();
    } else if (static_cast<SensorType>(chosen_sensor_type_for_sim) == SensorType::Network) { 
        signal = simulate_network_event();
    }
    else { 
        signal.sensor_type = SensorType::Keyboard;
        signal.timestamp_us = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()
        ).count();
        signal.active_app_id_hash = get_active_application_id_hash();
    }
    
    return signal;
}

AtomicSignal SimulatedAtomicSignalProcessor::simulate_mouse_event() {
    LOG_DEFAULT(LogLevel::DEBUG, "simulate_mouse_event: Basladi.\n"); 
    AtomicSignal signal;
    signal.timestamp_us = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count();
    signal.sensor_type = SensorType::Mouse;
    signal.active_app_id_hash = get_active_application_id_hash();

    std::uniform_int_distribution<> event_distrib(0, 4); 
    std::uniform_int_distribution<> coord_change_distrib_min(-10, 10); 
    std::uniform_int_distribution<> coord_change_distrib_large(-100, 100); 
    std::uniform_int_distribution<> button_distrib(0, 2); 

    signal.mouse_event_type = event_distrib(s_gen); 
    
    if (signal.mouse_event_type == 0 || signal.mouse_event_type == 3 || signal.mouse_event_type == 4) {
        LOG_DEFAULT(LogLevel::DEBUG, "simulate_mouse_event: Hareket olayı tespit edildi.\n"); 
        signal.mouse_event_type = 0; 
        
        int delta_x_val = coord_change_distrib_large(s_gen);
        int delta_y_val = coord_change_distrib_large(s_gen);

        if (delta_x_val == 0 && delta_y_val == 0) { 
            delta_x_val = (s_gen() % 2 == 0 ? 1 : -1) * (coord_change_distrib_min(s_gen) + 1); 
        }
        
        current_mouse_x += delta_x_val;
        current_mouse_y += delta_y_val;

        current_mouse_x = std::max(0, std::min(1920, current_mouse_x));
        current_mouse_y = std::max(0, std::min(1080, current_mouse_y)); 
        signal.mouse_x = current_mouse_x;
        signal.mouse_y = current_mouse_y;
        LOG_DEFAULT(LogLevel::DEBUG, "Mouse Hareket Sinyali Oluşturuldu: x=" << signal.mouse_x << ", y=" << signal.mouse_y << ", delta=(" << delta_x_val << "," << delta_y_val << ")\n"); 
    } else if (signal.mouse_event_type == 1) { 
        LOG_DEFAULT(LogLevel::DEBUG, "simulate_mouse_event: Tiklama olayı tespit edildi.\n"); 
        signal.mouse_button_state = (button_distrib(s_gen) == 0) ? 1 : 2; 
        signal.mouse_x = current_mouse_x; 
        signal.mouse_y = current_mouse_y;
        LOG_DEFAULT(LogLevel::DEBUG, "Mouse Tiklama Sinyali Oluşturuldu: x=" << signal.mouse_x << ", y=" << signal.mouse_y << ", button=" << static_cast<int>(signal.mouse_button_state) << "\n"); 
    } else { 
        LOG_DEFAULT(LogLevel::DEBUG, "simulate_mouse_event: Kaydırma olayı tespit edildi.\n"); 
        signal.mouse_button_state = 0; 
        
        int delta_x_scroll = coord_change_distrib_min(s_gen);
        int delta_y_scroll = coord_change_distrib_min(s_gen);
        if (std::abs(delta_x_scroll) < 1 && std::abs(delta_y_scroll) < 1) { 
            delta_y_scroll = (s_gen() % 2 == 0 ? 1 : -1) * (coord_change_distrib_min(s_gen) + 1); 
        }
        current_mouse_x += delta_x_scroll;
        current_mouse_y += delta_y_scroll;
        current_mouse_x = std::max(0, std::min(1920, current_mouse_x));
        current_mouse_y = std::max(0, std::min(1080, current_mouse_y));
        signal.mouse_x = current_mouse_x; 
        signal.mouse_y = current_mouse_y;
        LOG_DEFAULT(LogLevel::DEBUG, "Mouse Kaydirma Sinyali Oluşturuldu: x=" << signal.mouse_x << ", y=" << signal.mouse_y << "\n");
    }
    LOG_DEFAULT(LogLevel::DEBUG, "simulate_mouse_event: Bitti. Sinyal döndürülüyor.\n"); 
    return signal;
}

AtomicSignal SimulatedAtomicSignalProcessor::simulate_display_event() {
    LOG_DEFAULT(LogLevel::DEBUG, "simulate_display_event: Basladi.\n");
    AtomicSignal signal;
    signal.timestamp_us = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count();
    signal.sensor_type = SensorType::Display;
    signal.active_app_id_hash = get_active_application_id_hash();

    std::uniform_int_distribution<> brightness_distrib(50, 255); 
    std::uniform_int_distribution<> on_off_distrib(0, 20); 

    if (on_off_distrib(s_gen) == 0) { 
        signal.display_on = false;
        current_brightness = 0;
    } else {
        signal.display_on = true;
        current_brightness = brightness_distrib(s_gen);
    }
    signal.display_brightness = current_brightness;
    LOG_DEFAULT(LogLevel::DEBUG, "simulate_display_event: Parlaklik=" << static_cast<int>(signal.display_brightness) << ", Acik=" << (signal.display_on ? "Evet" : "Hayir") << "\n");
    LOG_DEFAULT(LogLevel::DEBUG, "simulate_display_event: Bitti.\n");
    return signal;
}

AtomicSignal SimulatedAtomicSignalProcessor::simulate_battery_event() {
    LOG_DEFAULT(LogLevel::DEBUG, "simulate_battery_event: Basladi.\n");
    AtomicSignal signal;
    signal.timestamp_us = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count();
    signal.sensor_type = SensorType::Battery;
    signal.active_app_id_hash = get_active_application_id_hash();

    std::uniform_int_distribution<> charge_change_distrib(-2, 2); 
    std::uniform_int_distribution<> charging_change_distrib(0, 10); 

    if (charging_change_distrib(s_gen) == 0) { 
        current_charging = !current_charging;
    }
    
    current_battery += charge_change_distrib(s_gen); 
    current_battery = std::max(0, std::min(100, static_cast<int>(current_battery))); 

    signal.battery_percentage = current_battery;
    signal.battery_charging = current_charging;
    LOG_DEFAULT(LogLevel::DEBUG, "simulate_battery_event: Pil=" << static_cast<int>(signal.battery_percentage) << "%, Sarj=" << (signal.battery_charging ? "Evet" : "Hayir") << "\n");
    LOG_DEFAULT(LogLevel::DEBUG, "simulate_battery_event: Bitti.\n");
    return signal;
}

AtomicSignal SimulatedAtomicSignalProcessor::simulate_network_event() {
    LOG_DEFAULT(LogLevel::DEBUG, "simulate_network_event: Basladi.\n");
    AtomicSignal signal;
    signal.timestamp_us = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count();
    signal.sensor_type = SensorType::Network;
    signal.active_app_id_hash = get_active_application_id_hash();

    std::uniform_int_distribution<> active_distrib(0, 5); 
    std::uniform_int_distribution<> bandwidth_distrib(1000, 10000); 
    std::uniform_int_distribution<> idle_bandwidth_distrib(0, 500); 

    if (active_distrib(s_gen) == 0) {
        current_network_active = !current_network_active;
    }
    signal.network_active = current_network_active;
    if (current_network_active) {
        signal.network_bandwidth_estimate = bandwidth_distrib(s_gen);
    } else {
        signal.network_bandwidth_estimate = idle_bandwidth_distrib(s_gen); 
    }
    LOG_DEFAULT(LogLevel::DEBUG, "simulate_network_event: Ag aktif=" << (signal.network_active ? "Evet" : "Hayir") << ", Bant genisliği=" << signal.network_bandwidth_estimate << " Kbps\n");
    LOG_DEFAULT(LogLevel::DEBUG, "simulate_network_event: Bitti.\n");
    return signal;
}
//--------------------------------
