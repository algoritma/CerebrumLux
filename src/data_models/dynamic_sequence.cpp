#include "dynamic_sequence.h"
#include "../core/logger.h"
#include "../core/utils.h" // get_current_timestamp_us, sensor_type_to_string için
#include "../brain/autoencoder.h" // CryptofigAutoencoder için
#include <numeric> // std::accumulate için
#include <algorithm> // std::min için
#include <vector> // std::vector için
#include <string> // std::string için

namespace CerebrumLux {

// DÜZELTİLDİ: Constructor tanımı kaldırıldı, çünkü .h dosyasında inline olarak tanımlıdır.
// DynamicSequence::DynamicSequence()
//     : id(""), timestamp_utc(std::chrono::system_clock::now()),
//       current_application_context(""), current_cpu_usage(0), current_ram_usage(0),
//       event_count(0), current_network_active(false), network_activity_level(0), network_protocol("")
// {
//     // DÜZELTİLDİ: Constructor'da statistical_features_vector ve latent_cryptofig_vector'ı boyutlandır ve sıfırla.
//     // Bu, ilk döngüde bile boş olmalarını engeller.
//     statistical_features_vector.assign(CerebrumLux::CryptofigAutoencoder::INPUT_DIM, 0.0f);
//     latent_cryptofig_vector.assign(CerebrumLux::CryptofigAutoencoder::LATENT_DIM, 0.0f);
// }


void DynamicSequence::update_from_signals(const std::deque<CerebrumLux::AtomicSignal>& signal_buffer, long long current_time_us, unsigned short app_hash, CerebrumLux::CryptofigAutoencoder& cryptofig_autoencoder_ref) {
    this->event_count = signal_buffer.size();
    this->timestamp_utc = std::chrono::system_clock::now();

    // statistical_features_vector'ı her zaman doldur ve boyutunu kontrol et.
    // Eğer constructor'da zaten boyutlandırılmışsa (ki öyle), burada sadece doğruluğunu kontrol ederiz.
    if (this->statistical_features_vector.empty() || this->statistical_features_vector.size() != CerebrumLux::CryptofigAutoencoder::INPUT_DIM) {
        this->statistical_features_vector.assign(CerebrumLux::CryptofigAutoencoder::INPUT_DIM, 0.0f);
        LOG_DEFAULT(LogLevel::WARNING, "DynamicSequence: statistical_features_vector yeniden boyutlandırıldı ve sıfırlandı.");
    }

    // Ağ durumu ve uygulama bağlamını sinyallerden çıkarım (örnek)
    this->current_network_active = false;
    this->network_activity_level = 0;
    this->current_application_context = "Unknown";
    this->current_cpu_usage = 0; // Sıfırla
    this->current_ram_usage = 0; // Sıfırla


    if (signal_buffer.empty()) {
        // Eğer buffer boşsa, istatistiksel özellikleri rastgele değerlerle doldur
        for (size_t i = 0; i < this->statistical_features_vector.size(); ++i) {
            this->statistical_features_vector[i] = CerebrumLux::SafeRNG::getInstance().get_float(0.0f, 1.0f);
        }
        LOG_DEFAULT(LogLevel::TRACE, "DynamicSequence: Boş sinyal buffer'ı için rastgele istatistiksel özellikler üretildi.");
    } else {
        // Örnek: Signal buffer'dan basit istatistikler çıkar
        float total_network_activity = 0.0f;
        int network_signals_count = 0;
        float total_cpu_usage_val = 0.0f;
        float total_ram_usage_val = 0.0f;
        int system_signals_count = 0;

        for (const auto& signal : signal_buffer) {
            try {
                if (signal.type == CerebrumLux::SensorType::Network) {
                    network_signals_count++;
                    total_network_activity += signal.network_activity_level;
                    this->current_network_active = signal.current_network_active;
                    this->network_protocol = signal.network_protocol;
                } else if (signal.type == CerebrumLux::SensorType::SystemEvent) { // DÜZELTİLDİ: SensorType::System yerine SystemEvent kullanıldı.
                    system_signals_count++;
                    size_t pos_cpu = signal.value.find("CPU:");
                    size_t pos_ram = signal.value.find("RAM:");
                    size_t pos_app = signal.value.find("APP:");

                    if (pos_cpu != std::string::npos) {
                        try {
                            total_cpu_usage_val += std::stoul(signal.value.substr(pos_cpu + 4));
                        } catch (const std::exception& e) {
                            LOG_DEFAULT(LogLevel::WARNING, "DynamicSequence: CPU değeri parse edilirken hata: " << e.what());
                        }
                    }
                    if (pos_ram != std::string::npos) {
                        try {
                            total_ram_usage_val += std::stoul(signal.value.substr(pos_ram + 4));
                        } catch (const std::exception& e) {
                            LOG_DEFAULT(LogLevel::WARNING, "DynamicSequence: RAM değeri parse edilirken hata: " << e.what());
                        }
                    }
                    if (pos_app != std::string::npos) {
                        this->current_application_context = signal.value.substr(pos_app + 4);
                    }
                }
            } catch (const std::exception& e) {
                LOG_DEFAULT(LogLevel::WARNING, "DynamicSequence: Sinyal işlenirken hata: " << e.what());
            }
        }

        // Hesaplanan istatistikleri statistical_features_vector'a atayın ve normalize edin
        if (network_signals_count > 0) this->network_activity_level = static_cast<int>(total_network_activity / network_signals_count);
        if (system_signals_count > 0) {
            this->current_cpu_usage = static_cast<unsigned int>(total_cpu_usage_val / system_signals_count);
            this->current_ram_usage = static_cast<unsigned int>(total_ram_usage_val / system_signals_count);
        }

        if (this->statistical_features_vector.size() >= 3) {
            this->statistical_features_vector[0] = static_cast<float>(this->current_cpu_usage) / 100.0f;
            this->statistical_features_vector[1] = static_cast<float>(this->current_ram_usage) / 1024.0f; // MB to GB (normalize)
            this->statistical_features_vector[2] = static_cast<float>(this->network_activity_level) / 100.0f;
            // Diğer elemanlar şimdilik 0.0f kalır veya daha karmaşık istatistiklerle doldurulur.
        }
    }


    // Autoencoder kullanarak latent_cryptofig_vector'ı güncelle
    if (!this->statistical_features_vector.empty()) {
        this->latent_cryptofig_vector = cryptofig_autoencoder_ref.encode(this->statistical_features_vector);
        if (this->latent_cryptofig_vector.size() != CerebrumLux::CryptofigAutoencoder::LATENT_DIM) {
            LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "DynamicSequence: Autoencoder'dan dönen latent_cryptofig_vector boyutu ( " << this->latent_cryptofig_vector.size() << ") beklenenden ( " << CerebrumLux::CryptofigAutoencoder::LATENT_DIM << ") farklı. Hata olası.");
            this->latent_cryptofig_vector.assign(CerebrumLux::CryptofigAutoencoder::LATENT_DIM, 0.0f); // Fallback
        }
    } else {
        // Bu durum artık constructor veya yukarıdaki mantık nedeniyle oluşmamalı ama bir fallback olarak kalsın.
        this->latent_cryptofig_vector.assign(CerebrumLux::CryptofigAutoencoder::LATENT_DIM, 0.0f);
        LOG_DEFAULT(LogLevel::WARNING, "DynamicSequence: statistical_features_vector beklenmedik şekilde boş. latent_cryptofig_vector varsayılan değerlerle dolduruldu.");
    }

    if (app_hash != 0 && this->current_application_context == "Unknown") {
        this->current_application_context = "AppHash_" + std::to_string(app_hash);
    }
    
    this->id = "seq_" + std::to_string(current_time_us) + "_" + std::to_string(app_hash);

    LOG_DEFAULT(LogLevel::TRACE, "DynamicSequence: Current sequence güncellendi. ID: " << this->id << ", Olay Sayısı: " << this->event_count << ", CPU: " << this->current_cpu_usage << "%, RAM: " << this->current_ram_usage << "MB, Context: " << this->current_application_context << ", Latent Cryptofig Boyutu: " << this->latent_cryptofig_vector.size());
}

} // namespace CerebrumLux