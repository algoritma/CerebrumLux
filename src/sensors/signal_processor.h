#ifndef CEREBRUM_LUX_SIGNAL_PROCESSOR_H
#define CEREBRUM_LUX_SIGNAL_PROCESSOR_H

#include "../core/enums.h" // Enum'lar için
#include "atomic_signal.h" // AtomicSignal için


class AtomicSignalProcessor; // İleri bildirim

// *** AtomicSignalProcessor: İşletim sistemi seviyesinde sinyal yakalama için soyut arayüz ***
// İleri bildirim
class AtomicSignalProcessor {
public:
    virtual ~AtomicSignalProcessor() = default; 
    virtual AtomicSignal capture_next_signal() = 0;
    virtual bool start_capture() = 0;
    virtual void stop_capture() = 0;
    virtual unsigned short get_active_application_id_hash() = 0;
};

#endif // CEREBRUM_LUX_SIGNAL_PROCESSOR_H