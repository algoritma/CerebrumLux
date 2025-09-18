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

class DynamicSequence;

// *** SequenceManager: Dinamik sinyal dizilerini yonetir ***
class SequenceManager {
public:

    SequenceManager();
    ~SequenceManager();

    // add_signal fonksiyonu CryptofigProcessor referansı alacak şekilde güncellendi
    bool add_signal(const AtomicSignal& signal, CryptofigProcessor& cryptofig_processor); 

    void step_simulation();

    std::deque<AtomicSignal> get_signal_buffer_copy() const; 

    // YENİ: current_sequence'e erişim için getter metodu
    DynamicSequence& get_current_sequence_ref() { return *current_sequence; }
    const DynamicSequence& get_current_sequence_ref() const { return *current_sequence; }

private:
    // update_current_sequence metodu CryptofigProcessor referansı alacak şekilde güncellendi
    void update_current_sequence(CryptofigProcessor& cryptofig_processor); 

    std::deque<AtomicSignal> signal_buffer; 
    size_t max_buffer_size = 100;           

    std::unique_ptr<DynamicSequence> current_sequence;

    uint64_t last_sequence_update_time_us;

};


#endif // CEREBRUM_LUX_SEQUENCE_MANAGER_H