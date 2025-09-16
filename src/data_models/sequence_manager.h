#ifndef CEREBRUM_LUX_SEQUENCE_MANAGER_H
#define CEREBRUM_LUX_SEQUENCE_MANAGER_H

#include <deque>
#include <memory>
#include <chrono>
#include "../core/enums.h"      // Enumlar için
#include "../core/utils.h"      // For convert_wstring_to_string (if needed elsewhere)
#include "dynamic_sequence.h"   // DynamicSequence için
#include "../sensors/atomic_signal.h" // AtomicSignal için
#include "../brain/cryptofig_processor.h" // CryptofigProcessor için ileri bildirim (parametre olarak kullanılacak)

// İleri bildirim
class CryptofigProcessor; 

// *** SequenceManager: Dinamik sinyal dizilerini yonetir ***
class SequenceManager {
private:
    std::deque<AtomicSignal> signal_buffer; 
    size_t max_buffer_size = 100;           
    // update_current_sequence metodu CryptofigProcessor referansı alacak şekilde güncellendi
    void update_current_sequence(CryptofigProcessor& cryptofig_processor); 
    long long last_sequence_update_time_us = 0; 

public:
    std::unique_ptr<DynamicSequence> current_sequence; 
    
    // add_signal fonksiyonu CryptofigProcessor referansı alacak şekilde güncellendi
    bool add_signal(const AtomicSignal& signal, CryptofigProcessor& cryptofig_processor); 

    SequenceManager();
    std::deque<AtomicSignal> get_signal_buffer_copy() const; 
};

#endif // CEREBRUM_LUX_SEQUENCE_MANAGER_H