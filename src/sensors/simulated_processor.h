#ifndef SIMULATED_PROCESSOR_H
#define SIMULATED_PROCESSOR_H

#include <string>
#include <random>
#include <chrono> // std::chrono::system_clock için
#include "../sensors/signal_processor.h" // CerebrumLux::SignalProcessor için
#include "../sensors/atomic_signal.h"   // CerebrumLux::AtomicSignal için
#include "../core/enums.h"             // CerebrumLux::SensorType, KeyType vb. için
#include "../core/utils.h"             // CerebrumLux::SafeRNG için

namespace CerebrumLux { // SimulatedAtomicSignalProcessor sınıfı bu namespace içine alınacak

class SimulatedAtomicSignalProcessor : public SignalProcessor {
public:
    SimulatedAtomicSignalProcessor();
    ~SimulatedAtomicSignalProcessor();

    bool start_capture() override;
    void stop_capture() override;
    
    // Temel simülasyon fonksiyonları
    CerebrumLux::AtomicSignal create_keyboard_signal(char ch);
    CerebrumLux::AtomicSignal capture_next_signal() override;
    unsigned short get_active_application_id_hash() override;

    // Ek simülasyon yardımcı fonksiyonları (Mevcut kodunuzdaki gibi ve yeni eklenenler)
    CerebrumLux::AtomicSignal simulate_mouse_event();
    CerebrumLux::AtomicSignal simulate_display_event();
    CerebrumLux::AtomicSignal simulate_battery_event();
    CerebrumLux::AtomicSignal simulate_network_event();
    CerebrumLux::AtomicSignal simulate_microphone_event();
    CerebrumLux::AtomicSignal simulate_camera_event();
    
    // Yeni eklenen simülasyon metotları için deklarasyonlar
    CerebrumLux::AtomicSignal simulate_system_event();
    CerebrumLux::AtomicSignal simulate_internal_ai_event();
    CerebrumLux::AtomicSignal simulate_application_context_change();

private:
    bool is_capturing;
    std::mt19937& generator; // SafeRNG'den alınan referans

    // Uygulama ID simülasyonu için dağıtım
    std::uniform_int_distribution<int> distrib_app_id;

    // Klavye simülasyonu için
    std::vector<char> keyboard_chars;
    std::uniform_int_distribution<size_t> distrib_keyboard_char;

    // Fare simülasyonu için
    std::uniform_int_distribution<int> distrib_mouse_delta;

    // Ağ simülasyonu için
    std::uniform_int_distribution<int> distrib_network_activity;
    std::uniform_int_distribution<int> distrib_network_protocol;
    std::vector<std::string> network_protocols;

    // Diğer simülasyonlar için dağıtımlar
    std::uniform_int_distribution<int> distrib_battery_level;
    std::uniform_int_distribution<int> distrib_display_status;
    
    // SensorType::Count enum değerini kullanmak için, enums.h'de olmalı.
    // enum class SensorType'ın sonuna özel bir "Count" üyesi eklememiz gerekebilir
    // veya dağıtım aralığını manuel olarak belirtmeliyiz.
    std::uniform_int_distribution<int> s_sensor_selection_distrib; // Constructor'da başlatılacak

    // Random string üretimi için (hash_string için)
    std::string generate_random_string(size_t length) const;
};

} // namespace CerebrumLux

#endif // SIMULATED_PROCESSOR_H