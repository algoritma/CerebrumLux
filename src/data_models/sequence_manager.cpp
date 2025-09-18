#include "sequence_manager.h" // Kendi başlık dosyasını dahil et
#include "../core/logger.h"      // LOG makrosu için
#include "../core/utils.h"       // Diğer yardımcı fonksiyonlar için
#include "../sensors/atomic_signal.h" // AtomicSignal için
#include "../brain/cryptofig_processor.h" // CryptofigProcessor için
#include "../brain/autoencoder.h" // CryptofigAutoencoder için (get_autoencoder çağrısı için)
#include <iostream>  // std::cout, std::cerr için
#include <sstream>   // std::stringstream için

// === SequenceManager Implementasyonlari ===


SequenceManager::SequenceManager() 
    : last_sequence_update_time_us(0),
      current_sequence(std::make_unique<DynamicSequence>()) 
{
    LOG_DEFAULT(LogLevel::INFO, "SequenceManager başlatıldı.");
}

SequenceManager::~SequenceManager() {
    LOG_DEFAULT(LogLevel::INFO, "SequenceManager sonlandırıldı.");
}

void SequenceManager::step_simulation() {
    LOG_DEFAULT(LogLevel::DEBUG, "SequenceManager step_simulation çalıştı.");
    // Adım simülasyonu buraya eklenebilir
}

// add_signal fonksiyonu Cryptofrocessor referansı alacak şekilde güncellendi
bool SequenceManager::add_signal(const AtomicSignal& signal, CryptofigProcessor& cryptofig_processor) { 
    LOG_DEFAULT(LogLevel::DEBUG, "add_signal: Sinyal tipi eklendi: " << static_cast<int>(signal.sensor_type) << ". Buffer boyutu: " << signal_buffer.size() << "\n"); 
    if (signal_buffer.size() >= max_buffer_size) {
        LOG_DEFAULT(LogLevel::TRACE, "add_signal: Buffer dolu, pop_front yapiliyor.\n"); 
        signal_buffer.pop_front(); 
    }
    signal_buffer.push_back(signal); 
    
    long long current_time_us = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count();

    if (current_time_us - last_sequence_update_time_us > 500000 || 
        (signal_buffer.size() % (max_buffer_size / 5) == 0 && signal_buffer.size() > 0)) {
        LOG_DEFAULT(LogLevel::DEBUG, "add_signal: Sequence update tetikleniyor.\n"); 
        update_current_sequence(cryptofig_processor); // CryptofigProcessor referansı ile çağrıldı
        last_sequence_update_time_us = current_time_us;
        LOG_DEFAULT(LogLevel::DEBUG, "add_signal: Sequence update tamamlandi, true döndürülüyor.\n"); 
        return true; 
    }
    LOG_DEFAULT(LogLevel::TRACE, "add_signal: Sequence update tetiklenmedi, false döndürülüyor.\n"); 
    return false;
}

// update_current_sequence fonksiyonu CryptofigProcessor referansı alacak şekilde güncellendi
void SequenceManager::update_current_sequence(CryptofigProcessor& cryptofig_processor) {
    LOG_DEFAULT(LogLevel::DEBUG, "update_current_sequence: Basladi.\n"); 
    unsigned short current_app_hash_for_sequence = 0;
    if (!signal_buffer.empty()) {
        current_app_hash_for_sequence = signal_buffer.back().active_app_id_hash;
    }
    if (!current_sequence) {
        LOG_DEFAULT(LogLevel::ERR_CRITICAL, "update_current_sequence: current_sequence nullptr! Yeniden oluşturuluyor.\n");
        current_sequence = std::make_unique<DynamicSequence>();
    }
    current_sequence->update_from_signals(signal_buffer, std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count(), current_app_hash_for_sequence, cryptofig_processor.get_autoencoder()); // Getter kullanıldı
     
    LOG_DEFAULT(LogLevel::DEBUG, "update_current_sequence: Bitti.\n"); 
}

std::deque<AtomicSignal> SequenceManager::get_signal_buffer_copy() const {
    return signal_buffer;
}


