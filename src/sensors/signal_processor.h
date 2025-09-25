#ifndef SIGNAL_PROCESSOR_H
#define SIGNAL_PROCESSOR_H

#include <string>
#include "../sensors/atomic_signal.h" // CerebrumLux::AtomicSignal için

namespace CerebrumLux { // SignalProcessor sınıfı bu namespace içine alınacak

// Tüm sinyal işleyicileri için temel arayüz
class SignalProcessor {
public:
    virtual ~SignalProcessor() = default; // Sanal yıkıcı

    // Sinyal yakalamayı başlat/durdur
    virtual bool start_capture() = 0;
    virtual void stop_capture() = 0;

    // Bir sonraki atomik sinyali yakalar
    virtual CerebrumLux::AtomicSignal capture_next_signal() = 0; // AtomicSignal güncellendi

    // Aktif uygulama ID'sinin hash'ini döndürür
    virtual unsigned short get_active_application_id_hash() = 0;
};

} // namespace CerebrumLux

#endif // SIGNAL_PROCESSOR_H