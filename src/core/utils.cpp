#include "utils.h" // Kendi başlık dosyasını dahil et
#include <iostream>  // std::cout, std::cerr için
#include <numeric>   // std::accumulate için
#include <cmath>     // std::sqrt, std::log10, std::fabs, std::exp için
#include <algorithm> // std::min, std::max, std::tolower için
// <locale>, <codecvt>, <stringapiset.h> utils.h'de dahil edildiği için burada tekrar gerekmez

// === Yardımcı Fonksiyon Implementasyonları (tümü std::string tabanlı) ===
/* 
// std::wstring'den std::string'e dönüştürme yardımcı fonksiyonu
// Bu, Windows API'si kullanarak daha robust bir dönüştürme sağlar.
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
*/

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

// Simülasyon amaçlı basit bir string hash fonksiyonu
unsigned short hash_string(const std::string& s) {
    unsigned short hash = 0;
    for (char c : s) {
        hash = (hash * 31 + c) % 65535; 
    }
    return hash;
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