#include "utils.h" // Kendi başlık dosyasını dahil et
#include <iostream>  // std::cout, std::cerr için
#include <numeric>   // std::accumulate için
#include <cmath>     // std::sqrt, std::log10, std::fabs, std::exp için
#include <algorithm> // std::iswalpha, std::iswdigit, std::min, std::max, std::towlower için
#ifdef _WIN32
#include <stringapiset.h> // _CRT_WIDE için (Windows özel)
#endif

// Helper function to convert wstring to string
std::string convert_wstring_to_string(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    using convert_type = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_type, wchar_t> converter;
    return converter.to_bytes(wstr);
}

// Mevcut zamanı formatlı bir string olarak döndüren yardımcı fonksiyon
std::string get_current_timestamp_str() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    #ifdef _WIN32
        struct tm buf;
        localtime_s(&buf, &in_time_t); // Güvenli localtime
        ss << std::put_time(&buf, "%Y-%m-%d %H:%M:%S");
    #else
        ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");
    #endif
    return ss.str();
}

// intent_to_string fonksiyonunun tanımı
std::string intent_to_string(UserIntent intent) {
    switch (intent) {
        case UserIntent::FastTyping: return "Hizli Yazma Modu";
        case UserIntent::Editing:    return "Duzenleme Modu";
        case UserIntent::IdleThinking: return "Dusunme/Ara Verme Modu";
        case UserIntent::Unknown:    return "Bilinmiyor";
        case UserIntent::None:       return "Hiçbiri"; 
        case UserIntent::Programming: return "Programlama Modu";
        case UserIntent::Gaming: return "Oyun Modu";
        case UserIntent::MediaConsumption: return "Medya Tuketimi Modu";
        case UserIntent::CreativeWork: return "Yaratici Calisma Modu";
        case UserIntent::Research: return "Arastirma Modu";
        case UserIntent::Communication: return "Iletisim Modu";
        case UserIntent::Count: return "Niyet Sayisi"; // Erişilmemeli, debug amaçlı
        default:                     return "Tanimlanmamis Niyet";
    }
}

// abstract_state_to_string fonksiyonunun tanımı
std::string abstract_state_to_string(AbstractState state) {
    switch (state) {
        case AbstractState::None: return "Hiçbiri";
        case AbstractState::HighProductivity: return "Yuksek Uretkenlik";
        case AbstractState::LowProductivity: return "Dusuk Uretkenlik";
        case AbstractState::Focused: return "Odaklanmis";
        case AbstractState::Distracted: return "Dikkati Dagilik";
        case AbstractState::PowerSaving: return "Guc Tasarrufu";
        case AbstractState::NormalOperation: return "Normal Calisma";
        case AbstractState::CreativeFlow: return "Yaratici Akis";
        case AbstractState::Debugging: return "Hata Ayiklama";
        case AbstractState::PassiveConsumption: return "Pasif Tuketim";
        case AbstractState::HardwareAnomaly: return "Donanim Anormalligi";
        case AbstractState::SeekingInformation: return "Bilgi Arayisi";
        case AbstractState::SocialInteraction: return "Sosyal Etkilesim";
        case AbstractState::Count: return "Durum Sayisi"; 
        default: return "Tanimlanmamis Durum";
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