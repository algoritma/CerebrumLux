#include "sequence_manager.h"
#include "../core/logger.h"
#include "../core/utils.h" // get_current_timestamp_us, sensor_type_to_string için
#include "../brain/autoencoder.h" // CryptofigAutoencoder için

namespace CerebrumLux {

SequenceManager::SequenceManager()
    : max_buffer_size(100), // Varsayılan buffer boyutu
      last_sequence_update_time_us(0)
{
    LOG_DEFAULT(LogLevel::INFO, "SequenceManager: Initialized.");
}

bool SequenceManager::add_signal(const AtomicSignal& signal, CryptofigProcessor& cryptofig_processor) {
    std::lock_guard<std::mutex> lock(buffer_mutex);
    signal_buffer.push_back(signal);
    if (signal_buffer.size() > max_buffer_size) {
        signal_buffer.pop_front(); // En eski sinyali kaldır
    }

    // Her yeni sinyalde ana seansı güncelle
    update_current_sequence(cryptofig_processor);
    
    LOG_DEFAULT(LogLevel::TRACE, "SequenceManager: Yeni sinyal eklendi. Tip: " << sensor_type_to_string(signal.type) << ", Değer: " << signal.value.substr(0, 20));
    return true;
}

void SequenceManager::process_oldest_signal(CryptofigProcessor& cryptofig_processor) {
    std::lock_guard<std::mutex> lock(buffer_mutex);
    if (!signal_buffer.empty()) {
        // En eski sinyali işleme ve buffer'dan kaldırma mantığı burada olabilir
        // Şimdilik sadece kaldırıyoruz.
        signal_buffer.pop_front();
        update_current_sequence(cryptofig_processor); // Sekansı tekrar güncelle
        LOG_DEFAULT(LogLevel::TRACE, "SequenceManager: En eski sinyal işlendi ve kaldırıldı.");
    }
}

const DynamicSequence& SequenceManager::get_current_sequence_ref() const {
    return current_sequence;
}

std::deque<AtomicSignal> SequenceManager::get_signal_buffer_copy() { // 'const' kaldırıldı
    std::lock_guard<std::mutex> lock(buffer_mutex);
    return signal_buffer;
}

void SequenceManager::update_current_sequence(CryptofigProcessor& cryptofig_processor) {
    // Current sequence'ı sinyal buffer'ındaki verilere göre güncelle
    current_sequence.update_from_signals(signal_buffer, get_current_timestamp_us(), 0, cryptofig_processor.get_autoencoder());
    LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "SequenceManager: Current sequence güncellendi.");
}

} // namespace CerebrumLux