#ifndef DYNAMIC_SEQUENCE_H
#define DYNAMIC_SEQUENCE_H

#include <string>
#include <vector>
#include <chrono>
#include <deque> // sinyal buffer için
#include "../sensors/atomic_signal.h" // CerebrumLux::AtomicSignal için
#include "../brain/autoencoder.h" // CerebrumLux::CryptofigAutoencoder için

namespace CerebrumLux { // DynamicSequence struct'ı bu namespace içine alınacak

// Dinamik Sekans yapısı
struct DynamicSequence {
    std::string id;
    std::chrono::system_clock::time_point timestamp_utc;
    std::string current_application_context; // Örneğin, "VSCode", "Chrome", "Game"
    unsigned int current_cpu_usage; // %0-100
    unsigned int current_ram_usage; // MB
    unsigned int event_count; // Bu sekans içinde işlenen olay sayısı

    std::vector<float> statistical_features_vector; // Zaman serisi istatistikleri (mean, variance, freq analysis, etc.)
    std::vector<float> latent_cryptofig_vector;     // Autoencoder'dan gelen sıkıştırılmış, semantik vektör

    // Ağ Aktivitesi Durumu (AtomicSignal'dan alınan en son değerler)
    bool current_network_active; // Ağ bağlantısı var mı?
    int network_activity_level; // (0-100)
    std::string network_protocol; // HTTP, HTTPS, TCP, UDP vb.

    DynamicSequence()
        : id(""), timestamp_utc(std::chrono::system_clock::now()),
          current_application_context(""), current_cpu_usage(0), current_ram_usage(0),
          event_count(0), current_network_active(false), network_activity_level(0), network_protocol("")
    {}

    // Signal buffer'dan bu seansı güncelle
    void update_from_signals(const std::deque<CerebrumLux::AtomicSignal>& signal_buffer, long long current_time_us, unsigned short app_hash, CerebrumLux::CryptofigAutoencoder& cryptofig_autoencoder_ref);
};

} // namespace CerebrumLux

#endif // DYNAMIC_SEQUENCE_H
