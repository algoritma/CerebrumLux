#ifndef CEREBRUM_LUX_UTILS_H
#define CEREBRUM_LUX_UTILS_H

#include <string>    // std::string için
#include <sstream>   // std::stringstream için
#include <chrono>    // std::chrono için (timestamp)
#include <iomanip>   // std::put_time için
#include <ctime>     // std::localtime için
#include <cerrno>    // errno için
#include <algorithm> // std::tolower (char versiyonu için)
#include <locale>    // std::locale için (sadece convert_wstring_to_string'de kullanılacak)
#include <codecvt>   // std::wstring_convert için (convert_wstring_to_string'de kullanılacak)
#include <random>    // YENİ: std::random_device, std::mt19937 için

// OpenSSL Başlıkları (utils.h içinde dahil ediliyor)
#include <openssl/crypto.h>
#include <openssl/buffer.h>

#ifdef _WIN32
#include <stringapiset.h> // WideCharToMultiByte için (Windows'a özel)
#endif

// Mesaj sistemi için (eğer MessageData ve MessageType burada tanımlıysa)
#include <queue>
#include <mutex>
#include <condition_variable>
#include "enums.h" // MessageData, MessageType, UserIntent, AbstractState, AIGoal, LogLevel için

// YENİ: Base64 encode/decode prototipleri (global fonksiyonlar olarak)
std::string base64_encode(const std::string& in);
std::string base64_decode(const std::string& in);

// YENİ: SafeRNG sınıfı tanımı (tekil rastgele sayı üreteci)
class SafeRNG {
public:
    static SafeRNG& get_instance(); // Tekil (singleton) erişim
    std::mt19937& get_generator(); // Rastgele sayı üreteci nesnesini döndürür

    SafeRNG(const SafeRNG&) = delete; // Kopyalamayı engelle
    void operator=(const SafeRNG&) = delete; // Atamayı engelle

private:
    SafeRNG(); // Kurucunun implementasyonu utils.cpp'ye taşındı
    std::mt19937 generator; // Rastgele sayı üreteci
};

// Yardımcı fonksiyon bildirimleri (tümü std::string tabanlı)
std::string get_current_timestamp_str();
std::string intent_to_string(UserIntent intent);
std::string abstract_state_to_string(AbstractState state);
std::string goal_to_string(AIGoal goal);
std::string action_to_string(AIAction action); // YENİ: action_to_string bildirimi eklendi
unsigned short hash_string(const std::string& s); // Artık std::string alıyor

// std::wstring'den std::string'e dönüştürme yardımcı fonksiyonu
// Bu, sadece harici std::wstring'ler geldiğinde veya bir std::wstring literalini dönüştürmek gerektiğinde kullanılacaktır.
// Mümkün olduğunca doğrudan std::string kullanmayı hedefliyoruz.
std::string convert_wstring_to_string(const std::wstring& wstr);

// MessageQueue sınıfının bildirimi (eğer utils.h içinde ise)
class MessageQueue {
public:
    void enqueue(MessageData data);
    MessageData dequeue();
    bool isEmpty();

private:
    std::queue<MessageData> queue;
    std::mutex mutex;
    std::condition_variable cv;
};


#endif // CEREBRUM_LUX_UTILS_H