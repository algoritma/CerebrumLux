#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include <random>
#include <chrono> // Time-related helper functions
#include <queue> // MessageQueue için
#include <mutex> // MessageQueue için
#include "enums.h" // Tüm CerebrumLux enum'ları için

namespace CerebrumLux { // Tüm yardımcı fonksiyonlar ve sınıflar bu namespace içine alınacak

// === Rastgele Sayı Üreteci (Singleton) ===
class SafeRNG {
public:
    static SafeRNG& get_instance();
    std::mt19937& get_generator();

private:
    SafeRNG();
    std::mt19937 generator;
    std::random_device rd;
};

// === Zamanla İlgili Yardımcı Fonksiyonlar ===
std::string get_current_timestamp_str();
long long get_current_timestamp_us(); // Mikrosaniye cinsinden zaman

// === Hash Fonksiyonları ===
unsigned short hash_string(const std::string& s); // YENİ EKLENDİ

// === Enum Dönüşüm Fonksiyonları ===
std::string intent_to_string(UserIntent intent);
std::string abstract_state_to_string(AbstractState state);
std::string goal_to_string(AIGoal goal);
std::string action_to_string(AIAction action);
std::string sensor_type_to_string(SensorType type);
std::string key_type_to_string(KeyType type);
std::string key_event_type_to_string(KeyEventType type);
std::string mouse_button_state_to_string(MouseButtonState state);

// === Mesaj Veri Yapısı ve Kuyruğu ===
// Mesaj türleri
enum class MessageType : unsigned char {
    Log,
    Command,
    Feedback,
    SensorData,
    Insight,
    CapsuleTransfer
};

struct MessageData {
    MessageType type;
    std::string content;
    long long timestamp; // Mesajın oluşturulma zamanı
    std::string source_module; // Mesajı gönderen modül

    // NLOHMANN_DEFINE_TYPE_INTRUSIVE(MessageData, type, content, timestamp, source_module) // Eğer JSON serileştirme isteniyorsa
};

class MessageQueue {
public:
    void enqueue(MessageData data);
    MessageData dequeue();
    bool isEmpty(); // 'const' kaldırıldı
    size_t size();  // 'const' kaldırıldı

private:
    std::queue<MessageData> queue;
    std::mutex mutex;
};

} // namespace CerebrumLux

#endif // UTILS_H
