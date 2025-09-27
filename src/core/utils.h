#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include <chrono> // Time functions
#include <random> // Random number generation
#include <iomanip> // std::put_time
#include <mutex>   // MessageQueue için
#include <queue>   // MessageQueue için

#include "enums.h" // Tüm CerebrumLux enumları için tam tanım

namespace CerebrumLux {

// === SafeRNG Implementasyonu (Singleton) ===
class SafeRNG {
public:
    static SafeRNG& get_instance(); // Singleton örneğini döndürür

    // Rastgele sayı üretim metotları
    int get_int(int min, int max);
    float get_float(float min, float max);
    
    // std::mt19937 generator'a erişim için public metot (GERİ EKLENDİ)
    std::mt19937& get_generator(); 

private:
    SafeRNG(); // Kurucu private
    SafeRNG(const SafeRNG&) = delete; // Kopyalama engellenir
    SafeRNG& operator=(const SafeRNG&) = delete; // Atama engellenir

    std::mt19937 generator; // Mersenne Twister motoru
    // std::random_device rd;  // Kaldırıldı: std::random_device'den kaynaklanan potansiyel sorunlar nedeniyle.
};

// === Zamanla İlgili Yardımcı Fonksiyonlar ===
std::string get_current_timestamp_str();
long long get_current_timestamp_us();

// === Hash Fonksiyonları ===
unsigned short hash_string(const std::string& s);

// === Enum Dönüşüm Fonksiyonları (Deklarasyonlar) ===
std::string intent_to_string(UserIntent intent);
std::string abstract_state_to_string(AbstractState state);
std::string goal_to_string(AIGoal goal);
std::string action_to_string(AIAction action);
std::string sensor_type_to_string(SensorType type);
std::string key_type_to_string(KeyType type);
std::string key_event_type_to_string(KeyEventType type);
std::string mouse_button_state_to_string(MouseButtonState state);

// === MessageQueue Tanımı ===
enum class MessageType { Log, Command, Feedback };
struct MessageData {
    MessageType type;
    std::string content;
    long long timestamp;
    std::string sender;
};

class MessageQueue {
public:
    void enqueue(MessageData data);
    MessageData dequeue();
    bool isEmpty();
    size_t size();

private:
    std::queue<MessageData> queue;
    std::mutex mutex;
};

} // namespace CerebrumLux

#endif // UTILS_H