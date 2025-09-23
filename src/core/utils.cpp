#include "utils.h" // Kendi başlık dosyasını dahil et
#include <iostream>  // std::cout, std::cerr için
#include <numeric>   // std::accumulate için
#include <cmath>     // std::sqrt, std::log10, std::fabs, std::exp için
#include <algorithm> // std::min, std::max, std::tolower için
#include "../core/logger.h" // LOG_DEFAULT için
// <locale>, <codecvt>, <stringapiset.h> utils.h'de dahil edildiği için burada tekrar gerekmez

// OpenSSL Başlıkları (Sadece utils.cpp içinde dahil ediliyor)
#include <openssl/crypto.h> // En genel OpenSSL başlığı, diğerlerini de içerebilir
#include <openssl/ssl.h>    // Bazı BIO fonksiyonları SSL'e bağımlı olabilir
#include <openssl/evp.h>    // EVP_* fonksiyonları için (encrypt/decrypt için)
#include <openssl/rand.h>   // RAND_bytes için
#include <openssl/err.h>    // ERR_print_errors_fp için
#include <openssl/bio.h>    // BIO_s_mem, BIO_new_mem_buf, BIO_free_all için
#include <openssl/buffer.h>   // <-- BIO_get_buf_mem burada

extern "C" {
// OpenSSL Başlıkları (Sadece utils.cpp içinde dahil ediliyor)
#include <openssl/crypto.h> // En genel OpenSSL başlığı, diğerlerini de içerebilir
#include <openssl/ssl.h>    // Bazı BIO fonksiyonları SSL'e bağımlı olabilir
#include <openssl/evp.h>    // EVP_* fonksiyonları için (encrypt/decrypt için)
#include <openssl/rand.h>   // RAND_bytes için
#include <openssl/err.h>    // ERR_print_errors_fp için
#include <openssl/bio.h>    // BIO_s_mem, BIO_new_mem_buf, BIO_free_all için
#include <openssl/buffer.h> // BUF_MEM ve BIO_get_buf_mem için KRİTİK!
}

#include <iostream>  // std::cout, std::cerr için
#include <numeric>   // std::accumulate için
#include <cmath>     // std::sqrt, std::log10, std::fabs, std::exp için
#include <algorithm> // std::min, std::max, std::tolower için
#include "../core/logger.h" // LOG_DEFAULT için
// <locale>, <codecvt>, <stringapiset.h> utils.h'de dahil edildiği için burada tekrar gerekmez

#include <iostream>  // std::cout, std::cerr için
#include <numeric>   // std::accumulate için
#include <cmath>     // std::sqrt, std::log10, std::fabs, std::exp için
#include <algorithm> // std::min, std::max, std::tolower için
#include "../core/logger.h" // LOG_DEFAULT için
// <locale>, <codecvt>, <stringapiset.h> utils.h'de dahil edildiği için burada tekrar gerekmez

// === SafeRNG Implementasyonları ===

// Kurucu: Doğrudan sistem saatini seed olarak kullan (random_device kaldırıldı)
SafeRNG::SafeRNG() {
    generator.seed(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    // LOG_DEFAULT burada kullanılamaz çünkü Logger henüz başlatılmamış olabilir.
    // main'de Logger başlatıldıktan sonra SafeRNG'nin başlatıldığından emin olunmalı.
}

// Tekil (singleton) erişim
SafeRNG& SafeRNG::get_instance() {
    static SafeRNG instance;
    return instance;
}

// Rastgele sayı üreteci nesnesini döndürür
std::mt19937& SafeRNG::get_generator() {
    return generator;
}


// === Yardımcı Fonksiyon Implementasyonları (tümü std::string tabanlı) ===

// Mevcut zamanı formatlı bir string olarak döndüren yardımcı fonksiyon
std::string get_current_timestamp_str() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
#ifdef _WIN32
    struct tm buf;
    localtime_s(&buf, &in_time_t); // Güvenli localtime (Windows)
    ss << std::put_time(&buf, "%Y-%m-%d %H:%M:%S"); 
#else
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S"); // Standart localtime
#endif
    return ss.str();
}

// intent_to_string fonksiyonunun tanımı
std::string intent_to_string(UserIntent intent) {
    switch (intent) {
        case UserIntent::None: return "Hiçbiri";
        case UserIntent::Unknown: return "Bilinmiyor";
        case UserIntent::FastTyping: return "Hızlı Yazma Modu";
        case UserIntent::Editing: return "Düzenleme Modu";
        case UserIntent::IdleThinking: return "Düşünme/Ara Verme Modu";
        case UserIntent::Programming: return "Programlama Modu";
        case UserIntent::Gaming: return "Oyun Modu";
        case UserIntent::MediaConsumption: return "Medya Tüketimi Modu";
        case UserIntent::CreativeWork: return "Yaratıcı Çalışma Modu";
        case UserIntent::Research: return "Araştırma Modu";
        case UserIntent::Communication: return "İletişim Modu";
        case UserIntent::VideoEditing: return "Video Düzenleme";
        case UserIntent::Browsing: return "İnternette Gezinme";
        case UserIntent::Reading: return "Okuma";
        case UserIntent::GeneralInquiry: return "Genel Sorgulama";
        case UserIntent::Count: return "Niyet Sayısı";
        default: return "Tanımlanmamış Niyet";
    }
}

// abstract_state_to_string fonksiyonunun tanımı
std::string abstract_state_to_string(AbstractState state) {
    switch (state) {
        case AbstractState::None: return "Hiçbiri";
        case AbstractState::HighProductivity: return "Yüksek Üretkenlik";
        case AbstractState::LowProductivity: return "Düşük Üretkenlik";
        case AbstractState::Focused: return "Odaklanmış";
        case AbstractState::Distracted: return "Dikkati Dağılmış";
        case AbstractState::PowerSaving: return "Güç Tasarrufu";
        case AbstractState::NormalOperation: return "Normal Çalışma";
        case AbstractState::CreativeFlow: return "Yaratıcı Akış";
        case AbstractState::Debugging: return "Hata Ayıklama";
        case AbstractState::PassiveConsumption: return "Pasif Tüketim";
        case AbstractState::HardwareAnomaly: return "Donanım Anormalliği";
        case AbstractState::SeekingInformation: return "Bilgi Arayışı";
        case AbstractState::SocialInteraction: return "Sosyal Etkileşim";
        case AbstractState::HighPerformance: return "Yüksek Performans";
        case AbstractState::FaultyHardware: return "Arızalı Donanım";
        case AbstractState::Count: return "Durum Sayısı";
        default: return "Tanımlanmamış Durum";
    }
}

// goal_to_string fonksiyonunun tanımı
std::string goal_to_string(AIGoal goal) {
    switch (goal) {
        case AIGoal::None: return "Yok";
        case AIGoal::OptimizeProductivity: return "Üretkenliği Optimize Etmek";
        case AIGoal::MaximizeBatteryLife: return "Batarya Ömrünü Maksimuma Çıkarmak";
        case AIGoal::ReduceDistractions: return "Dikkat Dağıtıcıları Azaltmak";
        case AIGoal::EnhanceCreativity: return "Yaratıcılığı Artırmak";
        case AIGoal::ImproveGamingExperience: return "Oyun Deneyimini İyileştirmek";
        case AIGoal::FacilitateResearch: return "Araştırmayı Kolaylaştırmak";
        case AIGoal::SelfImprovement: return "Kendi Kendini Geliştirmek";
        case AIGoal::Count: return "Hedef Sayısı";
        default: return "Bilinmeyen Hedef";
    }
}

// YENİ: action_to_string fonksiyonunun tanımı
std::string action_to_string(AIAction action) {
    switch (action) {
        case AIAction::DisableSpellCheck: return "Yazim denetimini devre disi birak";
        case AIAction::EnableCustomDictionary: return "Ozel sozlügü etkinlestir";
        case AIAction::ShowUndoHistory: return "Geri alma geçmisini goster";
        case AIAction::CompareVersions: return "Versiyonlari karsilastir";
        case AIAction::DimScreen: return "Ekrani karart";
        case AIAction::MuteNotifications: return "Bildirimleri sessize al";
        case AIAction::LaunchApplication: return "Uygulama baslat";
        case AIAction::OpenFile: return "Dosya aç";
        case AIAction::SetReminder: return "Hatirlatici kur";
        case AIAction::SimulateOSAction: return "OS eylemi simule et"; 
        case AIAction::SuggestBreak: return "Ara vermeyi öner";
        case AIAction::OptimizeForGaming: return "Oyun performansı için optimize et";
        case AIAction::EnableFocusMode: return "Odaklanma modunu etkinleştir";
        case AIAction::AdjustAudioVolume: return "Ses seviyesini ayarla";
        case AIAction::OpenDocumentation: return "Dokümantasyon aç";
        case AIAction::SuggestSelfImprovement: return "Kendi kendini geliştirme önerisi"; 
        case AIAction::None: return "Hiçbir eylem önerisi yok"; 
        case AIAction::Count: return "Eylem Sayisi"; 
        default: return "Tanimlanmamis eylem"; 
    }
}

// Simülasyon amaçlı basit bir string hash fonksiyonu
unsigned short hash_string(const std::string& s) {
    unsigned short hash = 0;
    for (char c : s) {
        hash = (hash * 31 + c) % 65535; 
    }
    return hash;
}

// std::wstring'den std::string'e dönüştürme yardımcı fonksiyonu
std::string convert_wstring_to_string(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
#ifdef _WIN32
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.length(), NULL, 0, NULL, NULL);
    std::string str_to(size_needed, 0); // allocate space
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.length(), &str_to[0], size_needed, NULL, NULL);
    return str_to;
#else
    // POSIX uyumlu sistemler için std::codecvt_utf8 (C++17'de deprecated)
    // veya daha modern alternatifler (örn. iconv, utfcpp) kullanılabilir.
    // Şimdilik C++17 öncesi veya uyarıyı tolere eden derleyiciler için bu yaklaşım.
    using convert_type = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_type, wchar_t> converter;
    return converter.to_bytes(wstr);
#endif
}

// YENİ: Base64 kodlama yardımcı fonksiyonu (BIO ile OpenSSL kullanımı)
/*
std::string base64_encode(const std::string& in) {
    BIO *b64, *bmem;
    BUF_MEM *bptr;

    b64 = BIO_new(BIO_f_base64());
    bmem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bmem);
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL); // Yeni satır karakterlerini engelle
    BIO_write(b64, in.c_str(), static_cast<int>(in.length()));
    BIO_flush(b64);
    BIO_get_buf_mem(b64, &bptr); // BUF_MEM için openssl/buffer.h gerekli

    std::string out(bptr->data, bptr->length);
    BIO_free_all(b64);
    return out;
}
*/
std::string base64_encode(const std::string& in) {
    BIO *b64, *bmem;
    BUF_MEM *bptr;

    b64 = BIO_new(BIO_f_base64());
    bmem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bmem);
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);

    BIO_write(b64, in.data(), static_cast<int>(in.size()));
    BIO_flush(b64);

    // Eski yöntem:
    // BIO_get_buf_mem(b64, &bptr);
    // std::string out(bptr->data, bptr->length);

    // Yeni uyumlu yöntem:
    char *data;
    long len = BIO_get_mem_data(bmem, &data);
    std::string out(data, len);

    BIO_free_all(b64);
    return out;
}


// YENİ: Base64 kod çözme yardımcı fonksiyonu (BIO ile OpenSSL kullanımı)
std::string base64_decode(const std::string& in) {
    BIO *b64, *bmem;
    char* buffer = nullptr;
    size_t length = 0;

    b64 = BIO_new(BIO_f_base64());
    bmem = BIO_new_mem_buf(in.c_str(), static_cast<int>(in.length()));
    b64 = BIO_push(b64, bmem);
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);

    length = in.length();
    buffer = (char*)OPENSSL_malloc(length + 1); // +1 null sonlandırma için
    if (!buffer) {
        // LOG_DEFAULT burada kullanılamaz, çünkü utils.cpp'nin logger'dan önce derlenmesi gerekebilir.
        // Hata durumunda boş string döndürmek yeterli.
        return "";
    }
    
    int decoded_len = BIO_read(b64, buffer, static_cast<int>(length));
    if (decoded_len < 0) {
        OPENSSL_free(buffer);
        BIO_free_all(b64);
        return "";
    }
    buffer[decoded_len] = '\0'; 

    std::string out(buffer, decoded_len);
    OPENSSL_free(buffer);
    BIO_free_all(b64);
    return out;
}


// MessageQueue sınıfı implementasyonları
void MessageQueue::enqueue(MessageData data) {
    {
        std::lock_guard<std::mutex> lock(mutex);
        queue.push(data);
    }
    cv.notify_one();
}

MessageData MessageQueue::dequeue() {
    std::unique_lock<std::mutex> lock(mutex);
    cv.wait(lock, [this]{ return !queue.empty(); });
    
    MessageData data = queue.front();
    queue.pop();
    return data;
}

bool MessageQueue::isEmpty() {
    std::lock_guard<std::mutex> lock(mutex);
    return queue.empty();
}