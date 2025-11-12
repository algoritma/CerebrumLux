#include "sequence_manager.h"
#include "../core/logger.h" // LOG_DEFAULT için
#include "../core/enums.h" // LogLevel için
#include "../core/utils.h" // get_current_timestamp_us, hash_string için
#include "../brain/cryptofig_processor.h" // CryptofigProcessor için
#include "../brain/autoencoder.h" // CryptofigAutoencoder::INPUT_DIM için
#include <iostream>
#include <algorithm> // std::min, std::max için

namespace CerebrumLux {

SequenceManager::SequenceManager() : max_buffer_size(100), last_sequence_update_time_us(0) {
    LOG_DEFAULT(LogLevel::INFO, "SequenceManager: Initialized.");
}

bool SequenceManager::add_signal(const AtomicSignal& signal, CryptofigProcessor& cryptofig_processor) {
    std::lock_guard<std::mutex> lock(buffer_mutex);
    LOG_DEFAULT(LogLevel::DEBUG, "SequenceManager::add_signal: Yeni sinyal eklendi. Type: " << sensor_type_to_string(signal.type) << ", Value: " << signal.value.substr(0, std::min((size_t)30, signal.value.length()))); // Kısaltılmış değer logu

    signal_buffer.push_back(signal);
    if (signal_buffer.size() > max_buffer_size) {
        signal_buffer.pop_front(); // En eski sinyali kaldır
        LOG_DEFAULT(LogLevel::TRACE, "SequenceManager::add_signal: Sinyal buffer boyutu aşıldı, en eski sinyal silindi.");
    }

    // Her sinyal eklendiğinde veya belirli aralıklarla mevcut sekansı güncelle
    update_current_sequence(cryptofig_processor);
    
    return true;
}

void SequenceManager::process_oldest_signal(CryptofigProcessor& cryptofig_processor) {
    std::lock_guard<std::mutex> lock(buffer_mutex);
    if (!signal_buffer.empty()) {
        AtomicSignal oldest_signal = signal_buffer.front();
        signal_buffer.pop_front();
        LOG_DEFAULT(LogLevel::DEBUG, "SequenceManager::process_oldest_signal: Eski sinyal işlendi ve kaldırıldı. ID: " << oldest_signal.id);
        // Burada eski sinyalin işlenmesi (örn. arşivlenmesi) yapılabilir.
    } else {
        LOG_DEFAULT(LogLevel::WARNING, "SequenceManager::process_oldest_signal: Sinyal buffer boş.");
    }
}

const DynamicSequence& SequenceManager::get_current_sequence_ref() const {
    return current_sequence;
}

std::deque<AtomicSignal> SequenceManager::get_signal_buffer_copy() {
    std::lock_guard<std::mutex> lock(buffer_mutex);
    return signal_buffer;
}

void SequenceManager::update_current_sequence(CryptofigProcessor& cryptofig_processor) {
    long long current_time_us = get_current_timestamp_us();
    // Sekansın ne kadar sıklıkla güncelleneceğine karar verilebilir.
    // Şimdilik her sinyal eklendiğinde güncelleniyor.

    current_sequence.id = "seq_" + std::to_string(current_time_us);
    current_sequence.timestamp_utc = std::chrono::system_clock::now();
    current_sequence.event_count = signal_buffer.size();
    current_sequence.current_application_context = "UnknownApp"; // Placeholder
    current_sequence.current_cpu_usage = 0; // Placeholder
    current_sequence.current_ram_usage = 0; // Placeholder
    
    // Basit bir sinyal analizi ve istatistiksel özellik çıkarma
    current_sequence.statistical_features_vector.clear();
    // Örnek: Rastgele veya dummy verilerle dolduralım şimdilik
    for (int i = 0; i < CryptofigAutoencoder::INPUT_DIM; ++i) {
        current_sequence.statistical_features_vector.push_back(SafeRNG::getInstance().get_float(0.0f, 1.0f));
    }
    LOG_DEFAULT(LogLevel::DEBUG, "SequenceManager::update_current_sequence: statistical_features_vector boyutu: " << current_sequence.statistical_features_vector.size());


    // Network aktivitesi gibi alanları en son sinyalden güncelle
    if (!signal_buffer.empty()) {
        const AtomicSignal& last_signal = signal_buffer.back();
        current_sequence.current_network_active = last_signal.current_network_active;
        current_sequence.network_activity_level = last_signal.network_activity_level;
        current_sequence.network_protocol = last_signal.network_protocol;
    } else {
        // Buffer boşsa varsayılan değerler
        current_sequence.current_network_active = false;
        current_sequence.network_activity_level = 0;
        current_sequence.network_protocol = "";
    }

    // CryptofigProcessor'ı çağırarak latent kriptofigi güncelle
    // sequence.statistical_features_vector artık boş olmayacağı için bu adım çalışmalıdır.
    // Ancak MetaEvolutionEngine'da const_cast kullanmak yerine, burada SequenceManager'dan mutable bir referans vermeliyiz
    // veya CryptofigProcessor'ın metodunu const DynamicSequence& alacak şekilde değiştirmeliyiz.
    // Şimdilik, SequenceManager'ın içindeki current_sequence'i modify ettiğimiz için doğrudan verebiliriz.
    try {
        cryptofig_processor.process_sequence(current_sequence, 0.01f); // 0.01f dummy learning rate
        LOG_DEFAULT(LogLevel::DEBUG, "SequenceManager::update_current_sequence: CryptofigProcessor ile DynamicSequence işlendi.");
        if (current_sequence.latent_cryptofig_vector.empty()) {
             LOG_DEFAULT(LogLevel::WARNING, "SequenceManager::update_current_sequence: CryptofigProcessor sonrası latent_cryptofig_vector hala boş.");
        } else {
             LOG_DEFAULT(LogLevel::TRACE, "SequenceManager::update_current_sequence: Latent kriptofig boyutu: " << current_sequence.latent_cryptofig_vector.size() << ", Örnek değer: " << current_sequence.latent_cryptofig_vector[0]);
        }
    } catch (const std::exception& e) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SequenceManager::update_current_sequence: CryptofigProcessor işlemi sırasında hata: " << e.what());
    } catch (...) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SequenceManager::update_current_sequence: CryptofigProcessor işlemi sırasında bilinmeyen hata.");
    }

    last_sequence_update_time_us = current_time_us;
    LOG_DEFAULT(LogLevel::DEBUG, "SequenceManager::update_current_sequence: DynamicSequence güncellendi. ID: " << current_sequence.id);
}

} // namespace CerebrumLux