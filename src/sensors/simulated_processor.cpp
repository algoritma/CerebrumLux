#include "simulated_processor.h"
#include "../core/logger.h"
#include "../core/utils.h" // get_current_timestamp_us, hash_string, SafeRNG için
#include <thread>
#include <chrono>

namespace CerebrumLux { // TÜM İMPLEMENTASYON BU NAMESPACE İÇİNDE OLACAK

// Statik üyenin tanımlanması (SensorType::Count'a göre)
// Constructor'da başlatılacağı için burada tanımlıyoruz.
// s_sensor_selection_distrib(0, static_cast<int>(CerebrumLux::SensorType::Count) - 1); // Constructor içinde olacak.

CerebrumLux::SimulatedAtomicSignalProcessor::SimulatedAtomicSignalProcessor()
    : is_capturing(false),
      generator(CerebrumLux::SafeRNG::get_instance().get_generator()),
      distrib_app_id(0, 4), // 5 farklı uygulama simülasyonu
      keyboard_chars({'a', 'b', 'c', 'd', 'e', ' ', '\n', '\t'}),
      distrib_keyboard_char(0, keyboard_chars.size() - 1),
      distrib_mouse_delta(-10, 10),
      distrib_network_activity(0, 100),
      distrib_network_protocol(0, 2), // 3 protokol simülasyonu
      network_protocols({"HTTP", "HTTPS", "TCP"}),
      distrib_battery_level(10, 100),
      distrib_display_status(0, 1), // 0:Normal, 1:Uyku
      s_sensor_selection_distrib(0, static_cast<int>(CerebrumLux::SensorType::Count) - 1) // SensorType::Count kullanıldı
{
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "SimulatedAtomicSignalProcessor: Initialized.");
}

CerebrumLux::SimulatedAtomicSignalProcessor::~SimulatedAtomicSignalProcessor() {
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "SimulatedAtomicSignalProcessor: Destructor called.");
}

bool CerebrumLux::SimulatedAtomicSignalProcessor::start_capture() {
    is_capturing = true;
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "Simulasyon baslatildi. Tuslara basin (Q ile ├ºikis).\n");
    return true;
}

void CerebrumLux::SimulatedAtomicSignalProcessor::stop_capture() {
    is_capturing = false;
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "Simulasyon durduruldu.\n");
}

unsigned short CerebrumLux::SimulatedAtomicSignalProcessor::get_active_application_id_hash() {
    int app_choice = distrib_app_id(generator);
    if (app_choice == 0) return CerebrumLux::hash_string("Tarayici");
    else if (app_choice == 1) return CerebrumLux::hash_string("MetinEditoru");
    else if (app_choice == 2) return CerebrumLux::hash_string("Terminal");
    else if (app_choice == 3) return CerebrumLux::hash_string("OyunUygulamasi");
    else if (app_choice == 4) return CerebrumLux::hash_string("VideoOynatici");
    else return CerebrumLux::hash_string("IDE");
}

CerebrumLux::AtomicSignal CerebrumLux::SimulatedAtomicSignalProcessor::create_keyboard_signal(char ch) {
    CerebrumLux::AtomicSignal signal;
    signal.id = generate_random_string(8);
    signal.type = CerebrumLux::SensorType::Keyboard;
    signal.timestamp_us = CerebrumLux::get_current_timestamp_us();
    signal.value = std::string(1, ch);

    if (std::isalnum(static_cast<unsigned char>(ch))) {
        signal.key_type = CerebrumLux::KeyType::Alphanumeric;
    } else if (ch == '\n' || ch == '\t') {
        signal.key_type = CerebrumLux::KeyType::Control;
    } else {
        signal.key_type = CerebrumLux::KeyType::Other;
    }
    signal.event_type = CerebrumLux::KeyEventType::Press; // Basitlik için sadece press

    return signal;
}


CerebrumLux::AtomicSignal CerebrumLux::SimulatedAtomicSignalProcessor::capture_next_signal() {
    // Rastgele bir sensör türü seç
    int sensor_type_int = s_sensor_selection_distrib(generator);
    CerebrumLux::SensorType selected_sensor_type = static_cast<CerebrumLux::SensorType>(sensor_type_int);

    switch (selected_sensor_type) {
        case CerebrumLux::SensorType::Keyboard: {
            char random_char = keyboard_chars[distrib_keyboard_char(generator)];
            return create_keyboard_signal(random_char);
        }
        case CerebrumLux::SensorType::Mouse:
            return simulate_mouse_event();
        case CerebrumLux::SensorType::Display:
            return simulate_display_event();
        case CerebrumLux::SensorType::Battery:
            return simulate_battery_event();
        case CerebrumLux::SensorType::Network:
            return simulate_network_event();
        case CerebrumLux::SensorType::Microphone:
            return simulate_microphone_event();
        case CerebrumLux::SensorType::Camera:
            return simulate_camera_event();
        case CerebrumLux::SensorType::InternalAI: {
            // Basit bir iç AI sinyali
            CerebrumLux::AtomicSignal signal;
            signal.id = generate_random_string(8);
            signal.type = CerebrumLux::SensorType::InternalAI;
            signal.timestamp_us = CerebrumLux::get_current_timestamp_us();
            signal.ai_internal_event_type = "Decision";
            signal.ai_internal_event_data = "Öğrenme döngüsü tamamlandı.";
            signal.confidence = static_cast<float>(CerebrumLux::SafeRNG::get_instance().get_generator()()) / CerebrumLux::SafeRNG::get_instance().get_generator().max();
            return signal;
        }
        case CerebrumLux::SensorType::SystemEvent: {
            // Basit bir sistem olayı
            CerebrumLux::AtomicSignal signal;
            signal.id = generate_random_string(8);
            signal.type = CerebrumLux::SensorType::SystemEvent;
            signal.timestamp_us = CerebrumLux::get_current_timestamp_us();
            signal.system_event_type = "CPU_LOAD_HIGH";
            signal.system_event_data = "CPU yükü %85";
            return signal;
        }
        case CerebrumLux::SensorType::EyeTracker:
            // Placeholder: EyeTracker simülasyonu
            LOG_DEFAULT(CerebrumLux::LogLevel::WARNING, "EyeTracker simülasyonu henüz implemente edilmedi.");
            break;
        case CerebrumLux::SensorType::Count: // Count'ı simüle etmiyoruz
            break;
    }

    // Varsayılan boş sinyal
    CerebrumLux::AtomicSignal empty_signal;
    empty_signal.id = generate_random_string(8);
    empty_signal.type = CerebrumLux::SensorType::InternalAI; // Varsayılan olarak AI sinyali
    empty_signal.value = "Empty Signal";
    empty_signal.timestamp_us = CerebrumLux::get_current_timestamp_us();
    return empty_signal;
}

CerebrumLux::AtomicSignal CerebrumLux::SimulatedAtomicSignalProcessor::simulate_mouse_event() {
    CerebrumLux::AtomicSignal signal;
    signal.id = generate_random_string(8);
    signal.type = CerebrumLux::SensorType::Mouse;
    signal.timestamp_us = CerebrumLux::get_current_timestamp_us();
    signal.mouse_x += distrib_mouse_delta(generator);
    signal.mouse_y += distrib_mouse_delta(generator);
    signal.mouse_delta_x = distrib_mouse_delta(generator);
    signal.mouse_delta_y = distrib_mouse_delta(generator);
    
    // Rastgele düğme durumu
    int btn_choice = std::uniform_int_distribution<int>(0, 3)(generator);
    if (btn_choice == 0) signal.mouse_button_state = CerebrumLux::MouseButtonState::Left;
    else if (btn_choice == 1) signal.mouse_button_state = CerebrumLux::MouseButtonState::Right;
    else if (btn_choice == 2) signal.mouse_button_state = CerebrumLux::MouseButtonState::Middle;
    else signal.mouse_button_state = CerebrumLux::MouseButtonState::None;
    
    signal.value = "X:" + std::to_string(signal.mouse_x) + ",Y:" + std::to_string(signal.mouse_y);
    return signal;
}

CerebrumLux::AtomicSignal CerebrumLux::SimulatedAtomicSignalProcessor::simulate_display_event() {
    CerebrumLux::AtomicSignal signal;
    signal.id = generate_random_string(8);
    signal.type = CerebrumLux::SensorType::Display;
    signal.timestamp_us = CerebrumLux::get_current_timestamp_us();
    int status = distrib_display_status(generator);
    if (status == 0) {
        signal.system_event_type = "DisplayStatus";
        signal.system_event_data = "Normal";
        signal.value = "Normal";
    } else {
        signal.system_event_type = "DisplayStatus";
        signal.system_event_data = "Sleep";
        signal.value = "Sleep";
    }
    return signal;
}

CerebrumLux::AtomicSignal CerebrumLux::SimulatedAtomicSignalProcessor::simulate_battery_event() {
    CerebrumLux::AtomicSignal signal;
    signal.id = generate_random_string(8);
    signal.type = CerebrumLux::SensorType::Battery;
    signal.timestamp_us = CerebrumLux::get_current_timestamp_us();
    signal.system_event_type = "BatteryLevel";
    signal.network_activity_level = distrib_battery_level(generator); // Seviye için network_activity_level'ı kullanıyoruz
    signal.value = std::to_string(signal.network_activity_level) + "%";
    return signal;
}

CerebrumLux::AtomicSignal CerebrumLux::SimulatedAtomicSignalProcessor::simulate_network_event() {
    CerebrumLux::AtomicSignal signal;
    signal.id = generate_random_string(8);
    signal.type = CerebrumLux::SensorType::Network;
    signal.timestamp_us = CerebrumLux::get_current_timestamp_us();
    signal.current_network_active = (CerebrumLux::SafeRNG::get_instance().get_generator()() % 2 == 0); // GÜNCELLENDİ
    signal.network_activity_level = distrib_network_activity(generator);
    signal.network_protocol = network_protocols[distrib_network_protocol(generator)];
    signal.value = signal.network_protocol + ":" + std::to_string(signal.network_activity_level);
    return signal;
}

CerebrumLux::AtomicSignal CerebrumLux::SimulatedAtomicSignalProcessor::simulate_microphone_event() {
    CerebrumLux::AtomicSignal signal;
    signal.id = generate_random_string(8);
    signal.type = CerebrumLux::SensorType::Microphone;
    signal.timestamp_us = CerebrumLux::get_current_timestamp_us();
    signal.value = "Simulated Audio Clip Hash: " + generate_random_string(16);
    // Gerçek ses verisi yerine hash
    return signal;
}

CerebrumLux::AtomicSignal CerebrumLux::SimulatedAtomicSignalProcessor::simulate_camera_event() {
    CerebrumLux::AtomicSignal signal;
    signal.id = generate_random_string(8);
    signal.type = CerebrumLux::SensorType::Camera;
    signal.timestamp_us = CerebrumLux::get_current_timestamp_us();
    signal.value = "Simulated Image Hash: " + generate_random_string(16);
    // Gerçek görüntü verisi yerine hash
    return signal;
}


// Yardımcı fonksiyon
std::string CerebrumLux::SimulatedAtomicSignalProcessor::generate_random_string(size_t length) const {
    const std::string CHARS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    std::string s = "";
    s.reserve(length);
    for (size_t i = 0; i < length; ++i) {
        s += CHARS[CerebrumLux::SafeRNG::get_instance().get_generator()() % CHARS.length()]; // GÜNCELLENDİ
    }
    return s;
}

} // namespace CerebrumLux