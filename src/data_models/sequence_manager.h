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
#include "../core/enums.h" // UserIntent için eklendi

namespace CerebrumLux { // SequenceManager sınıfı bu namespace içine alınacak

class SequenceManager {
public:
    SequenceManager();

    bool add_signal(const CerebrumLux::AtomicSignal& signal, CerebrumLux::CryptofigProcessor& cryptofig_processor);
    void process_oldest_signal(CerebrumLux::CryptofigProcessor& cryptofig_processor);
    const DynamicSequence& get_current_sequence_ref() const;
    std::deque<CerebrumLux::AtomicSignal> get_signal_buffer_copy();
    
    // YENİ EKLENDİ: Kullanıcı girdisini geçmişe ekler
    void add_user_input(const std::string& input);

private:
    std::deque<CerebrumLux::AtomicSignal> signal_buffer;
    mutable std::mutex buffer_mutex; // mutable eklendi çünkü get_current_sequence_ref const, ama mutex'i kilitlemesi gerekiyor.
                                     // VEYA get_current_sequence_ref'de mutex kullanmıyorsak gerek yok.
                                     // Kodunuza baktım, get_current_sequence_ref mutex kullanmıyor, o yüzden mutable şart değil ama iyi pratik.
                                     // Ancak mevcut kodunuzda 'buffer_mutex' var, onu kullanacağız.
    DynamicSequence current_sequence; // Dinamik sekans
    size_t max_buffer_size; // Sinyal buffer'ının maksimum boyutu
    long long last_sequence_update_time_us; // Son sekans güncelleme zamanı
    
    const size_t MAX_HISTORY_SIZE = 20; // Chat geçmişi sınırı

    // Yardımcı fonksiyonlar
    void update_current_sequence(CerebrumLux::CryptofigProcessor& cryptofig_processor);
};

} // namespace CerebrumLux

#endif // SEQUENCE_MANAGER_H