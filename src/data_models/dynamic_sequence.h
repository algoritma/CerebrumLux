#ifndef CEREBRUM_LUX_DYNAMIC_SEQUENCE_H
#define CEREBRUM_LUX_DYNAMIC_SEQUENCE_H

#include <vector>
#include <deque>
#include <chrono>
#include "../core/enums.h"    // Enumlar için
#include "../core/utils.h"    // For convert_wstring_to_string (if needed elsewhere)
#include "../brain/autoencoder.h" // CryptofigAutoencoder için ileri bildirim (sadece bildirim için)
#include "../sensors/atomic_signal.h" // AtomicSignal'ın tam tanımı için EKLEDİM

// İleri bildirim (CryptofigAutoencoder için hala geçerli, çünkü sadece referansı kullanılıyor)
class CryptofigAutoencoder; 

// *** DynamicSequence: Niyet paternlerinin tasiyicisi (Bizim 'Fuzyon Kriptofig' prototipimiz) ***
struct DynamicSequence { 
    std::vector<float> statistical_features_vector; // İstatistiksel özellikleri tutacak vektör (önceki cryptofig_vector)
    std::vector<float> latent_cryptofig_vector;     // Autoencoder tarafından üretilen düşük boyutlu, latent kriptofig
    long long last_updated_us;             
    
    float avg_keystroke_interval;          
    float keystroke_variability;           
    float alphanumeric_ratio;              
    float control_key_frequency;           
    
    float mouse_movement_intensity; 
    float mouse_click_frequency;    
    float avg_brightness;           
    float battery_status_change;    
    float network_activity_level;   

    unsigned short current_app_hash;       

    unsigned char current_battery_percentage; 
    bool current_battery_charging;
    bool current_display_on;      // Ekran açık/kapalı durumu
    bool current_network_active;  // Ağ bağlantısı aktif/pasif durumu

    // YENİ EKLENECEK SENSÖR VERİLERİ İÇİN ALANLAR
    float avg_audio_level_db;
    float avg_audio_frequency_hz;
    float speech_detection_ratio; // buffer içinde konuşma algılanan sinyal oranı
    unsigned short dominant_audio_environment_hash; // buffer içindeki en baskın ses ortamı

    float avg_ambient_light_lux;
    float face_detection_ratio; // buffer içinde yüz algılanan sinyal oranı
    float motion_detection_ratio; // buffer içinde hareket algılanan sinyal oranı
    unsigned short avg_object_count;
    unsigned short dominant_emotion_hash; // buffer içindeki en baskın duygu

    DynamicSequence();
    // update_from_signals metodu CryptofigAutoencoder referansı alacak şekilde güncellendi
    void update_from_signals(const std::deque<AtomicSignal>& signal_buffer, long long current_time_us, unsigned short app_hash, CryptofigAutoencoder& autoencoder);
};

#endif // CEREBRUM_LUX_DYNAMIC_SEQUENCE_H