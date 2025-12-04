#include "simulated_processor.h"
#include "../core/logger.h"
#include "../core/utils.h" // get_current_timestamp_us, hash_string, SafeRNG için
#include <thread>
#include <chrono>
#include <algorithm> // std::min için
#include <vector> // std::vector için
#include <cctype> // std::isalnum için

namespace CerebrumLux { // TÜM İMPLEMENTASYON BU NAMESPACE İÇİNDE OLACAK

CerebrumLux::SimulatedAtomicSignalProcessor::SimulatedAtomicSignalProcessor()
    : is_capturing(false),
      generator(CerebrumLux::SafeRNG::getInstance().get_generator()),
      distrib_app_id(0, 4), // 5 farklı uygulama simülasyonu
      keyboard_chars({'a', 'b', 'c', 'd', 'e', ' ', '\n', '\t'}),
      distrib_keyboard_char(0, keyboard_chars.size() - 1),
      distrib_mouse_delta(-10, 10),
      distrib_network_activity(0, 100),
      distrib_network_protocol(0, 2), // 3 protokol simülasyonu
      network_protocols({"HTTP", "HTTPS", "TCP"}),
      distrib_battery_level(10, 100),
      distrib_display_status(0, 1), // 0:Normal, 1:Uyku
      s_sensor_selection_distrib(0, static_cast<int>(CerebrumLux::SensorType::Count) - 1) 
{
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "SimulatedAtomicSignalProcessor: Initialized.");
}

CerebrumLux::SimulatedAtomicSignalProcessor::~SimulatedAtomicSignalProcessor() {
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "SimulatedAtomicSignalProcessor: Destructor called.");
}

bool CerebrumLux::SimulatedAtomicSignalProcessor::start_capture() {
    is_capturing = true;
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "Simulasyon baslatildi.\n");
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
    signal.confidence = CerebrumLux::SafeRNG::getInstance().get_float(0.7f, 1.0f);

    if (std::isalnum(static_cast<unsigned char>(ch))) {
        signal.key_type = CerebrumLux::KeyType::Alphanumeric;
    } else if (ch == '\n' || ch == '\t') {
        signal.key_type = CerebrumLux::KeyType::Control;
    } else {
        signal.key_type = CerebrumLux::KeyType::Other;
    }
    signal.event_type = CerebrumLux::KeyEventType::Press;

    return signal;
}


CerebrumLux::AtomicSignal CerebrumLux::SimulatedAtomicSignalProcessor::capture_next_signal() {
    int sensor_type_int = s_sensor_selection_distrib(generator);
    CerebrumLux::SensorType selected_sensor_type; // YENİ: Değişken burada deklare edildi.

    // Eğer seçilen tip Count ise, varsayılan bir tipi seç (örn. InternalAI)
    if (sensor_type_int == static_cast<int>(CerebrumLux::SensorType::Count)) {
        selected_sensor_type = CerebrumLux::SensorType::InternalAI;
    } else {
        selected_sensor_type = static_cast<CerebrumLux::SensorType>(sensor_type_int);
    }
    
    // LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "SimulatedAtomicSignalProcessor: Sensor tipi seçimi: " << sensor_type_to_string(selected_sensor_type)); // Detaylı log

    switch (selected_sensor_type) {
        case CerebrumLux::SensorType::Keyboard: {
            char random_char = keyboard_chars[distrib_keyboard_char(generator)];
            LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "SimulatedAtomicSignalProcessor: Klavye sinyali üretildi. Key: " << random_char);
            return create_keyboard_signal(random_char);
        }
        case CerebrumLux::SensorType::Mouse: {
            CerebrumLux::AtomicSignal signal = simulate_mouse_event();
            LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "SimulatedAtomicSignalProcessor: Fare sinyali üretildi. Pos: (" << signal.mouse_x << "," << signal.mouse_y << ")");
            return signal;
        }
        case CerebrumLux::SensorType::Display: {
            CerebrumLux::AtomicSignal signal = simulate_display_event();
            LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "SimulatedAtomicSignalProcessor: Ekran sinyali üretildi. Status: " << signal.system_event_data);
            return signal;
        }
        case CerebrumLux::SensorType::Battery: {
            CerebrumLux::AtomicSignal signal = simulate_battery_event();
            LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "SimulatedAtomicSignalProcessor: Batarya sinyali üretildi. Level: " << signal.network_activity_level << "%");
            return signal;
        }
        case CerebrumLux::SensorType::Network: {
            CerebrumLux::AtomicSignal signal = simulate_network_event();
            LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "SimulatedAtomicSignalProcessor: Ağ sinyali üretildi. Protocol: " << signal.network_protocol << ", Level: " << signal.network_activity_level << "%");
            return signal;
        }
        case CerebrumLux::SensorType::Microphone: {
            CerebrumLux::AtomicSignal signal = simulate_microphone_event();
            LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "SimulatedAtomicSignalProcessor: Mikrofon sinyali üretildi.");
            return signal;
        }
        case CerebrumLux::SensorType::Camera: {
            CerebrumLux::AtomicSignal signal = simulate_camera_event();
            LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "SimulatedAtomicSignalProcessor: Kamera sinyali üretildi.");
            return signal;
        }
        case CerebrumLux::SensorType::InternalAI: {
            CerebrumLux::AtomicSignal signal = simulate_internal_ai_event();
            LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "SimulatedAtomicSignalProcessor: AI İçsel sinyal üretildi. Type: " << signal.ai_internal_event_type);
            return signal;
        }
        case CerebrumLux::SensorType::SystemEvent: {
             CerebrumLux::AtomicSignal signal = simulate_system_event();
             LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "SimulatedAtomicSignalProcessor: Sistem olayı sinyali üretildi. Type: " << signal.system_event_type);
             return signal;
        }
        case CerebrumLux::SensorType::EyeTracker: {
            CerebrumLux::AtomicSignal signal = simulate_eye_tracker_event();
            LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "SimulatedAtomicSignalProcessor: Göz takip sinyali üretildi. Pos: (" << signal.mouse_x << "," << signal.mouse_y << ")");
            return signal;
        }
        case CerebrumLux::SensorType::BioSensor: {
            CerebrumLux::AtomicSignal signal = simulate_bio_sensor_event();
            LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "SimulatedAtomicSignalProcessor: Biyosensör sinyali üretildi. Data: " << signal.value);
            return signal;
        }
        case CerebrumLux::SensorType::Count: 
            LOG_DEFAULT(CerebrumLux::LogLevel::WARNING, "SimulatedAtomicSignalProcessor: SensorType::Count seçildi, varsayılan bir iç AI sinyali üretiliyor.");
            return simulate_internal_ai_event();
    }

    CerebrumLux::AtomicSignal empty_signal;
    empty_signal.id = generate_random_string(8);
    empty_signal.type = CerebrumLux::SensorType::InternalAI; 
    empty_signal.value = "Empty or Unhandled Signal Type";
    empty_signal.timestamp_us = CerebrumLux::get_current_timestamp_us();
    empty_signal.confidence = 0.0f; 
    LOG_DEFAULT(CerebrumLux::LogLevel::WARNING, "SimulatedAtomicSignalProcessor: Tanimsiz veya islenemeyen sensor tipi icin bos sinyal uretildi.");
    return empty_signal;
}

CerebrumLux::AtomicSignal SimulatedAtomicSignalProcessor::simulate_mouse_event() {
    CerebrumLux::AtomicSignal signal;
    signal.id = generate_random_string(8);
    signal.type = CerebrumLux::SensorType::Mouse;
    signal.timestamp_us = CerebrumLux::get_current_timestamp_us();
    signal.confidence = CerebrumLux::SafeRNG::getInstance().get_float(0.6f, 0.9f);

    signal.mouse_x = CerebrumLux::SafeRNG::getInstance().get_int(0, 1920);
    signal.mouse_y = CerebrumLux::SafeRNG::getInstance().get_int(0, 1080);
    signal.mouse_delta_x = distrib_mouse_delta(generator);
    signal.mouse_delta_y = distrib_mouse_delta(generator);
    
    int btn_choice = std::uniform_int_distribution<int>(0, 3)(generator);
    if (btn_choice == 0) signal.mouse_button_state = CerebrumLux::MouseButtonState::Left;
    else if (btn_choice == 1) signal.mouse_button_state = CerebrumLux::MouseButtonState::Right;
    else if (btn_choice == 2) signal.mouse_button_state = CerebrumLux::MouseButtonState::Middle;
    else signal.mouse_button_state = CerebrumLux::MouseButtonState::None;
    
    signal.value = "X:" + std::to_string(signal.mouse_x) + ",Y:" + std::to_string(signal.mouse_y);
    return signal;
}

CerebrumLux::AtomicSignal SimulatedAtomicSignalProcessor::simulate_display_event() {
    CerebrumLux::AtomicSignal signal;
    signal.id = generate_random_string(8);
    signal.type = CerebrumLux::SensorType::Display;
    signal.timestamp_us = CerebrumLux::get_current_timestamp_us();
    signal.confidence = CerebrumLux::SafeRNG::getInstance().get_float(0.8f, 1.0f);
    
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

CerebrumLux::AtomicSignal SimulatedAtomicSignalProcessor::simulate_battery_event() {
    CerebrumLux::AtomicSignal signal;
    signal.id = generate_random_string(8);
    signal.type = CerebrumLux::SensorType::Battery;
    signal.timestamp_us = CerebrumLux::get_current_timestamp_us();
    signal.confidence = CerebrumLux::SafeRNG::getInstance().get_float(0.7f, 1.0f);
    
    signal.system_event_type = "BatteryLevel";
    signal.network_activity_level = distrib_battery_level(generator);
    signal.value = std::to_string(signal.network_activity_level) + "%";
    return signal;
}

CerebrumLux::AtomicSignal SimulatedAtomicSignalProcessor::simulate_network_event() {
    CerebrumLux::AtomicSignal signal;
    signal.id = generate_random_string(8);
    signal.type = CerebrumLux::SensorType::Network;
    signal.timestamp_us = CerebrumLux::get_current_timestamp_us();
    signal.confidence = CerebrumLux::SafeRNG::getInstance().get_float(0.5f, 0.9f);

    signal.current_network_active = (CerebrumLux::SafeRNG::getInstance().get_int(0, 1) == 1);
    signal.network_activity_level = distrib_network_activity(generator);
    signal.network_protocol = network_protocols[distrib_network_protocol(generator)];
    signal.value = signal.network_protocol + ":" + std::to_string(signal.network_activity_level);
    return signal;
}

CerebrumLux::AtomicSignal SimulatedAtomicSignalProcessor::simulate_microphone_event() {
    CerebrumLux::AtomicSignal signal;
    signal.id = generate_random_string(8);
    signal.type = CerebrumLux::SensorType::Microphone;
    signal.timestamp_us = CerebrumLux::get_current_timestamp_us();
    signal.confidence = CerebrumLux::SafeRNG::getInstance().get_float(0.3f, 0.8f);
    
    signal.value = "Simulated Audio Clip Hash: " + generate_random_string(16);
    return signal;
}

CerebrumLux::AtomicSignal SimulatedAtomicSignalProcessor::simulate_camera_event() {
    CerebrumLux::AtomicSignal signal;
    signal.id = generate_random_string(8);
    signal.type = CerebrumLux::SensorType::Camera;
    signal.timestamp_us = CerebrumLux::get_current_timestamp_us();
    signal.confidence = CerebrumLux::SafeRNG::getInstance().get_float(0.3f, 0.8f);
    
    signal.value = "Simulated Image Hash: " + generate_random_string(16);
    return signal;
}

CerebrumLux::AtomicSignal SimulatedAtomicSignalProcessor::simulate_system_event() {
    AtomicSignal signal;
    signal.id = generate_random_string(8);
    signal.type = SensorType::SystemEvent;
    signal.timestamp_us = get_current_timestamp_us();
    std::uniform_int_distribution<int> event_dist(0, 1);
    switch (event_dist(generator)) {
        case 0: signal.system_event_type = "OS_STATUS_UPDATE"; signal.system_event_data = "CPU: " + std::to_string(SafeRNG::getInstance().get_int(10, 90)) + "%, RAM: " + std::to_string(SafeRNG::getInstance().get_int(20, 80)) + "%"; break;
        case 1: signal.system_event_type = "LOW_BATTERY"; signal.system_event_data = "Battery at " + std::to_string(CerebrumLux::SafeRNG::getInstance().get_int(5, 20)) + "%"; break;
        case 2: signal.system_event_type = "APPLICATION_CRASH"; signal.system_event_data = "App 'X' crashed."; break;
    }
    signal.value = signal.system_event_type;

    return signal;
}

CerebrumLux::AtomicSignal SimulatedAtomicSignalProcessor::simulate_internal_ai_event() {
    AtomicSignal signal;
    signal.id = generate_random_string(8);
    signal.type = SensorType::InternalAI;
    signal.timestamp_us = get_current_timestamp_us();
    signal.confidence = SafeRNG::getInstance().get_float(0.4f, 1.0f);

    std::uniform_int_distribution<int> event_dist(0, 2);
    switch (event_dist(generator)) {
        case 0: signal.ai_internal_event_type = "INTENT_DETECTED"; signal.ai_internal_event_data = "Intent: Programming, Confidence: 0.85"; break;
        case 1: signal.ai_internal_event_type = "LEARNING_COMPLETED"; signal.ai_internal_event_data = "Topic: Quantum Physics"; break;
        case 2: signal.ai_internal_event_type = "GOAL_CHANGED"; signal.ai_internal_event_data = "New Goal: MaximizeLearning"; break;
    }
    signal.value = signal.ai_internal_event_type;

    return signal;
}

CerebrumLux::AtomicSignal SimulatedAtomicSignalProcessor::simulate_application_context_change() {
    AtomicSignal signal;
    signal.id = generate_random_string(8);
    signal.type = SensorType::SystemEvent; 
    signal.timestamp_us = get_current_timestamp_us();
    signal.confidence = SafeRNG::getInstance().get_float(0.8f, 1.0f);

    std::vector<std::string> apps = {"VSCode", "Chrome", "Photoshop", "Unity3D", "Spotify"};
    std::uniform_int_distribution<int> app_dist(0, apps.size() - 1);
    signal.system_event_type = "APP_CONTEXT_CHANGE";
    signal.system_event_data = "Active App: " + apps[app_dist(generator)];
    signal.value = signal.system_event_data;

    return signal;
}

CerebrumLux::AtomicSignal SimulatedAtomicSignalProcessor::simulate_eye_tracker_event() {
    AtomicSignal signal;
    signal.id = generate_random_string(8);
    signal.type = SensorType::EyeTracker;
    signal.timestamp_us = get_current_timestamp_us();
    signal.confidence = SafeRNG::getInstance().get_float(0.6f, 1.0f);
    signal.mouse_x = SafeRNG::getInstance().get_int(0, 1920);
    signal.mouse_y = SafeRNG::getInstance().get_int(0, 1080);
    signal.value = "Gaze X:" + std::to_string(signal.mouse_x) + ", Y:" + std::to_string(signal.mouse_y);
    return signal;
}

CerebrumLux::AtomicSignal SimulatedAtomicSignalProcessor::simulate_bio_sensor_event() {
    AtomicSignal signal;
    signal.id = generate_random_string(8);
    signal.type = SensorType::BioSensor;
    signal.timestamp_us = get_current_timestamp_us();
    signal.confidence = SafeRNG::getInstance().get_float(0.5f, 1.0f);
    std::uniform_int_distribution<int> bio_type_dist(0, 1);
    if (bio_type_dist(generator) == 0) {
        signal.value = "HeartRate:" + std::to_string(SafeRNG::getInstance().get_int(60, 120));
    } else {
        signal.value = "SkinConductance:" + std::to_string(SafeRNG::getInstance().get_float(0.1f, 5.0f));
    }
    return signal;
}

std::string CerebrumLux::SimulatedAtomicSignalProcessor::generate_random_string(size_t length) const {
    const std::string CHARS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    std::string s = "";
    s.reserve(length);
    for (size_t i = 0; i < length; ++i) {
        s += CHARS[CerebrumLux::SafeRNG::getInstance().get_int(0, CHARS.length() - 1)];
    }
    return s;
}

} // namespace CerebrumLux