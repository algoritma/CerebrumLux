#include "dynamic_sequence.h"
#include "../core/logger.h"
#include "../core/utils.h" // get_current_timestamp_us, sensor_type_to_string için
#include "../brain/autoencoder.h" // CryptofigAutoencoder için

namespace CerebrumLux {

// ... (Diğer metotlar) ...

void DynamicSequence::update_from_signals(const std::deque<CerebrumLux::AtomicSignal>& signal_buffer, long long current_time_us, unsigned short app_hash, CerebrumLux::CryptofigAutoencoder& cryptofig_autoencoder_ref) { // Parametre adı güncellendi
    // Burada signal_buffer'dan istatistiksel özellikleri çıkarma ve
    // autoencoder kullanarak latent kriptofig vektörünü güncelleme lojiği yer alacak.
    // Şimdilik sadece placeholder olarak bırakıyoruz.

    // Örnek: Basitçe buffer'daki sinyal sayısını event_count olarak alalım
    this->event_count = signal_buffer.size();
    this->timestamp_utc = std::chrono::system_clock::now(); // Güncelleme zamanı

    // statistical_features_vector'ı buradan doldurun (gerçek implementasyonda daha karmaşık olur)
    // Şimdilik sadece varsayılan değerlerle dolduruyoruz
    this->statistical_features_vector.assign(CerebrumLux::CryptofigAutoencoder::INPUT_DIM, 0.0f);
    
    // Autoencoder kullanarak latent_cryptofig_vector'ı güncelle
    if (!this->statistical_features_vector.empty()) {
        this->latent_cryptofig_vector = cryptofig_autoencoder_ref.encode(this->statistical_features_vector); // Parametre referansı kullanıldı
    } else {
        this->latent_cryptofig_vector.assign(CerebrumLux::CryptofigAutoencoder::LATENT_DIM, 0.0f);
    }

    // Ağ durumu ve uygulama bağlamını sinyallerden çıkarım (örnek)
    this->current_network_active = false;
    this->network_activity_level = 0;
    this->current_application_context = "Unknown";

    for (const auto& signal : signal_buffer) {
        if (signal.type == CerebrumLux::SensorType::Network) {
            this->current_network_active = signal.current_network_active;
            this->network_activity_level = signal.network_activity_level;
            this->network_protocol = signal.network_protocol;
        }
        // Daha karmaşık mantık burada eklenebilir
    }

    LOG_DEFAULT(LogLevel::TRACE, "DynamicSequence: Current sequence güncellendi. ID: " << this->id << ", Olay Sayısı: " << this->event_count);
}

} // namespace CerebrumLux
