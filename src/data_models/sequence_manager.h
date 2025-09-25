#ifndef SEQUENCE_MANAGER_H
#define SEQUENCE_MANAGER_H

#include <string>
#include <vector>
#include <deque>
#include <mutex>
#include <chrono> // Time functions
#include "../sensors/atomic_signal.h" // CerebrumLux::AtomicSignal için
#include "../brain/cryptofig_processor.h" // CerebrumLux::CryptofigProcessor için
#include "../data_models/dynamic_sequence.h" // CerebrumLux::DynamicSequence için
#include "../core/enums.h" // UserIntent için eklendi (görev isteğine göre)

namespace CerebrumLux { // SequenceManager sınıfı bu namespace içine alınacak

class SequenceManager {
public:
    SequenceManager();

    bool add_signal(const CerebrumLux::AtomicSignal& signal, CerebrumLux::CryptofigProcessor& cryptofig_processor); // AtomicSignal güncellendi
    void process_oldest_signal(CerebrumLux::CryptofigProcessor& cryptofig_processor);
    const DynamicSequence& get_current_sequence_ref() const;
    std::deque<CerebrumLux::AtomicSignal> get_signal_buffer_copy(); // 'const' kaldırıldı

private:
    std::deque<CerebrumLux::AtomicSignal> signal_buffer; // AtomicSignal güncellendi
    std::mutex buffer_mutex;
    DynamicSequence current_sequence; // Dinamik sekans
    size_t max_buffer_size; // Sinyal buffer'ının maksimum boyutu
    long long last_sequence_update_time_us; // Son sekans güncelleme zamanı

    // Yardımcı fonksiyonlar
    void update_current_sequence(CerebrumLux::CryptofigProcessor& cryptofig_processor);
};

} // namespace CerebrumLux

#endif // SEQUENCE_MANAGER_H