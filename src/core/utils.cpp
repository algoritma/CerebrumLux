#include "utils.h" // Kendi başlık dosyasını dahil et
#include <iostream>  // std::wcout, std::wcerr için
#include <numeric>   // std::accumulate için (aslında hash_string için gerekli değil, ancak önceki cpp'de vardı)
#include <cmath>     // std::sqrt, std::log10, std::fabs, std::exp için (hash_string için de değil)
#include <algorithm> // std::iswalpha, std::iswdigit, std::min, std::max, std::towlower için
#include <locale>    // Geniş karakter fonksiyonlari için
#ifdef _WIN32
#include <stringapiset.h> // _CRT_WIDE için (Windows özel)
#endif

//mesajlamşa için
#include "utils.h"
#include <iostream>

// Bu dosyada tüm genel yardımcı fonksiyon implementasyonları yer alacak:
// log_level_to_string, get_current_timestamp_wstr, hash_string



// Fonksiyon implementasyonları (utils.h'deki bildirimlere uygun olarak)

/*
//kaldırıldı. logger.cpp de aynı tanım var. burada tekrarı hataya sebep oluyor
// log_level_to_string fonksiyonunun tanımı
std::wstring log_level_to_string(LogLevel level) {
    switch (level) {
        case LogLevel::SILENT:   return L"SILENT";
        case LogLevel::ERR_CRITICAL:    return L"ERROR";
        case LogLevel::WARNING:  return L"WARNING";
        case LogLevel::INFO:     return L"INFO";
        case LogLevel::DEBUG:    return L"DEBUG";
        case LogLevel::TRACE:    return L"TRACE";
        default:                 return L"UNKNOWN";
    }
}
*/

// Mevcut zamanı formatlı bir string olarak döndüren yardımcı fonksiyon
std::wstring get_current_timestamp_wstr() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    
    std::wstringstream ss;
    #ifdef _WIN32
        struct tm buf;
        localtime_s(&buf, &in_time_t); // Güvenli localtime
        ss << std::put_time(&buf, L"%Y-%m-%d %H:%M:%S");
    #else
        ss << std::put_time(std::localtime(&in_time_t), L"%Y-%m-%d %H:%M:%S");
    #endif
    return ss.str();
}

// intent_to_string fonksiyonunun tanımı
std::wstring intent_to_string(UserIntent intent) {
    switch (intent) {
        case UserIntent::FastTyping: return L"Hizli Yazma Modu";
        case UserIntent::Editing:    return L"Duzenleme Modu";
        case UserIntent::IdleThinking: return L"Dusunme/Ara Verme Modu";
        case UserIntent::Unknown:    return L"Bilinmiyor";
        case UserIntent::None:       return L"Hiçbiri"; 
        case UserIntent::Programming: return L"Programlama Modu";
        case UserIntent::Gaming: return L"Oyun Modu";
        case UserIntent::MediaConsumption: return L"Medya Tuketimi Modu";
        case UserIntent::CreativeWork: return L"Yaratici Calisma Modu";
        case UserIntent::Research: return L"Arastirma Modu";
        case UserIntent::Communication: return L"Iletisim Modu";
        case UserIntent::Count: return L"Niyet Sayisi"; // Erişilmemeli, debug amaçlı
        default:                     return L"Tanimlanmamis Niyet";
    }
}

// abstract_state_to_string fonksiyonunun tanımı
std::wstring abstract_state_to_string(AbstractState state) {
    switch (state) {
        case AbstractState::None: return L"Hiçbiri";
        case AbstractState::HighProductivity: return L"Yuksek Uretkenlik";
        case AbstractState::LowProductivity: return L"Dusuk Uretkenlik";
        case AbstractState::Focused: return L"Odaklanmis";
        case AbstractState::Distracted: return L"Dikkati Dagilik";
        case AbstractState::PowerSaving: return L"Guc Tasarrufu";
        case AbstractState::NormalOperation: return L"Normal Calisma";
        case AbstractState::CreativeFlow: return L"Yaratici Akis";
        case AbstractState::Debugging: return L"Hata Ayiklama";
        case AbstractState::PassiveConsumption: return L"Pasif Tuketim";
        case AbstractState::HardwareAnomaly: return L"Donanim Anormalligi";
        case AbstractState::SeekingInformation: return L"Bilgi Arayisi";
        case AbstractState::SocialInteraction: return L"Sosyal Etkilesim";
        case AbstractState::Count: return L"Durum Sayisi"; 
        default: return L"Tanimlanmamis Durum";
    }
}

// goal_to_string fonksiyonunun tanımı
std::wstring goal_to_string(AIGoal goal) {
    switch (goal) {
        case AIGoal::None: return L"Yok";
        case AIGoal::OptimizeProductivity: return L"Üretkenliği Optimize Etmek";
        case AIGoal::MaximizeBatteryLife: return L"Batarya Ömrünü Maksimuma Çıkarmak";
        case AIGoal::ReduceDistractions: return L"Dikkat Dağıtıcıları Azaltmak";
        case AIGoal::EnhanceCreativity: return L"Yaratıcılığı Artırmak";
        case AIGoal::ImproveGamingExperience: return L"Oyun Deneyimini İyileştirmek";
        case AIGoal::FacilitateResearch: return L"Araştırmayı Kolaylaştırmak";
        case AIGoal::SelfImprovement: return L"Kendi Kendini Geliştirmek";
        case AIGoal::Count: return L"Hedef Sayısı";
        default: return L"Bilinmeyen Hedef";
    }
}

// Simülasyon amaçlı basit bir string hash fonksiyonu
unsigned short hash_string(const std::wstring& s) {
    unsigned short hash = 0;
    for (wchar_t c : s) {
        hash = (hash * 31 + c) % 65535; 
    }
    return hash;
}

// MessageQueue sınıfın  tanımı
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