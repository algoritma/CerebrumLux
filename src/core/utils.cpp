#include "utils.h"
#include <iomanip>
#include <sstream>
#include <random>
#include <thread>
#include "logger.h" // LOG_DEFAULT için
#include <ctime>   // std::time_t, std::localtime için

namespace CerebrumLux { // TÜM İMPLEMENTASYON BU NAMESPACE İÇİNDE OLACAK

// === SafeRNG Implementasyonu ===
SafeRNG& SafeRNG::getInstance() {
    static SafeRNG instance;
    return instance;
}

// SafeRNG kurucusu: std::random_device yerine zaman tabanlı tohumlama
SafeRNG::SafeRNG() : generator(std::chrono::system_clock::now().time_since_epoch().count()) {
    LOG_DEFAULT(LogLevel::DEBUG, "SafeRNG: Initialized with time-based seed.");
}

// std::mt19937 generator'a erişim için public metot implementasyonu (GERİ EKLENDİ)
std::mt19937& SafeRNG::get_generator() {
    return generator;
}

// YENİ EKLENDİ: shutdown metodu
void SafeRNG::shutdown() {
    // mt19937 için özel bir kapatma işlemi gerekmiyor. İleride kaynak eklenirse buraya eklenebilir.
}

int SafeRNG::get_int(int min, int max) {
    std::uniform_int_distribution<int> dist(min, max);
    return dist(generator);
}

float SafeRNG::get_float(float min, float max) {
    std::uniform_real_distribution<float> dist(min, max);
    return dist(generator);
}

// ✅ YENİ: Gaussian dağılımından rastgele float üretme
float SafeRNG::get_gaussian_float(float mean, float stddev) {
    std::normal_distribution<float> dist(mean, stddev);
    return dist(generator);
}

// === Zamanla İlgili Yardımcı Fonksiyonlar ===
std::string get_current_timestamp_str() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm bt;
#ifdef _WIN32
    localtime_s(&bt, &t); // Windows için güvenli versiyon
#else
    // POSIX/Linux için, thread-safe olmayan localtime yerine localtime_r önerilir.
    // Ancak MinGW genellikle localtime'ı güvenli bir bağlamda kullanır.
    bt = *std::localtime(&t); 
#endif
    
    std::ostringstream oss;
    oss << std::put_time(&bt, "%Y-%m-%d %H:%M:%S");
    oss << '.' << std::setw(3) << std::setfill('0') << ms.count();
    return oss.str();
}

long long get_current_timestamp_us() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count();
}

// ✅ YENİ: Benzersiz ID üretme fonksiyonu
std::string generate_unique_id() {
    // Unix epoch'tan itibaren mikrosaniye cinsinden zaman damgası + küçük bir rastgele sayı
    return std::to_string(get_current_timestamp_us()) + "_" + std::to_string(SafeRNG::getInstance().get_int(1000, 9999));
}

// === Hash Fonksiyonları ===
unsigned short hash_string(const std::string& s) {
    std::hash<std::string> hasher;
    return static_cast<unsigned short>(hasher(s) % 65521);
}

// === Enum Dönüşüm Fonksiyonları ===
std::string intent_to_string(UserIntent intent) {
    switch (intent) {
        case UserIntent::Undefined: return "Undefined";
        case UserIntent::Question: return "Question";
        case UserIntent::Command: return "Command";
        case UserIntent::Statement: return "Statement";
        case UserIntent::FeedbackPositive: return "FeedbackPositive";
        case UserIntent::FeedbackNegative: return "FeedbackNegative";
        case UserIntent::Greeting: return "Greeting";
        case UserIntent::Farewell: return "Farewell";
        case UserIntent::RequestInformation: return "RequestInformation";
        case UserIntent::ExpressEmotion: return "ExpressEmotion";
        case UserIntent::Confirm: return "Confirm";
        case UserIntent::Deny: return "Deny";
        case UserIntent::Elaborate: return "Elaborate";
        case UserIntent::Clarify: return "Clarify";
        case UserIntent::CorrectError: return "CorrectError";
        case UserIntent::InquireCapability: return "InquireCapability";
        case UserIntent::ShowStatus: return "ShowStatus";
        case UserIntent::ExplainConcept: return "ExplainConcept";
        case UserIntent::FastTyping: return "FastTyping";
        case UserIntent::Programming: return "Programming";
        case UserIntent::Gaming: return "Gaming";
        case UserIntent::MediaConsumption: return "MediaConsumption";
        case UserIntent::CreativeWork: return "CreativeWork";
        case UserIntent::Research: return "Research";
        case UserIntent::Communication: return "Communication";
        case UserIntent::Editing: return "Editing";
        case UserIntent::Unknown: return "Unknown";
        default: return "UnknownIntent";
    }
}

std::string abstract_state_to_string(AbstractState state) {
    switch (state) {
        case AbstractState::Idle: return "Idle";
        case AbstractState::AwaitingInput: return "AwaitingInput";
        case AbstractState::ProcessingInput: return "ProcessingInput";
        case AbstractState::Responding: return "Responding";
        case AbstractState::Learning: return "Learning";
        case AbstractState::ExecutingTask: return "ExecutingTask";
        case AbstractState::Error: return "Error";
        case AbstractState::Calibrating: return "Calibrating";
        case AbstractState::SelfReflecting: return "SelfReflecting";
        case AbstractState::SuspiciousActivity: return "SuspiciousActivity";
        case AbstractState::NormalOperation: return "NormalOperation";
        case AbstractState::SeekingInformation: return "SeekingInformation";
        case AbstractState::PowerSaving: return "PowerSaving";
        case AbstractState::FaultyHardware: return "FaultyHardware";
        case AbstractState::Distracted: return "Distracted";
        case AbstractState::Focused: return "Focused";
        case AbstractState::HighProductivity: return "HighProductivity";
        case AbstractState::LowProductivity: return "LowProductivity";
        case AbstractState::Debugging: return "Debugging";
        case AbstractState::CreativeFlow: return "CreativeFlow";
        case AbstractState::PassiveConsumption: return "PassiveConsumption";
        case AbstractState::SocialInteraction: return "SocialInteraction";
        case AbstractState::Undefined: return "Undefined";
        default: return "UnknownState";
    }
}

std::string goal_to_string(AIGoal goal) {
    switch (goal) {
        case AIGoal::OptimizeProductivity: return "OptimizeProductivity";
        case AIGoal::MinimizeErrors: return "MinimizeErrors";
        case AIGoal::MaximizeLearning: return "MaximizeLearning";
        case AIGoal::EnsureSecurity: return "EnsureSecurity";
        case AIGoal::MaintainUserSatisfaction: return "MaintainUserSatisfaction";
        case AIGoal::ConserveResources: return "ConserveResources";
        case AIGoal::ExploreNewKnowledge: return "ExploreNewKnowledge";
        case AIGoal::UndefinedGoal: return "UndefinedGoal";
        default: return "UnknownGoal";
    }
}

std::string action_to_string(AIAction action) {
    switch (action) {
        case AIAction::None: return "None";
        case AIAction::RespondToUser: return "RespondToUser";
        case AIAction::SuggestSelfImprovement: return "SuggestSelfImprovement";
        case AIAction::AdjustLearningRate: return "AdjustLearningRate";
        case AIAction::RequestMoreData: return "RequestMoreData";
        case AIAction::QuarantineCapsule: return "QuarantineCapsule";
        case AIAction::InitiateHandshake: return "InitiateHandshake";
        case AIAction::PerformWebSearch: return "PerformWebSearch";
        case AIAction::UpdateKnowledgeBase: return "UpdateKnowledgeBase";
        case AIAction::MonitorPerformance: return "MonitorPerformance";
        case AIAction::CalibrateSensors: return "CalibrateSensors";
        case AIAction::ExecutePlan: return "ExecutePlan";
        case AIAction::PrioritizeTask: return "PrioritizeTask";
        case AIAction::RefactorCode: return "RefactorCode";
        case AIAction::SuggestResearch: return "SuggestResearch";
        case AIAction::MaximizeLearning: return "MaximizeLearning";
        default: return "UnknownAction";
    }
}

AIAction string_to_action(const std::string& action_str) {
    if (action_str == "None") return AIAction::None;
    if (action_str == "RespondToUser") return AIAction::RespondToUser;
    if (action_str == "SuggestSelfImprovement") return AIAction::SuggestSelfImprovement;
    if (action_str == "AdjustLearningRate") return AIAction::AdjustLearningRate;
    if (action_str == "RequestMoreData") return AIAction::RequestMoreData;
    if (action_str == "QuarantineCapsule") return AIAction::QuarantineCapsule;
    if (action_str == "InitiateHandshake") return AIAction::InitiateHandshake;
    if (action_str == "PerformWebSearch") return AIAction::PerformWebSearch;
    if (action_str == "UpdateKnowledgeBase") return AIAction::UpdateKnowledgeBase;
    if (action_str == "MonitorPerformance") return AIAction::MonitorPerformance;
    if (action_str == "CalibrateSensors") return AIAction::CalibrateSensors;
    if (action_str == "ExecutePlan") return AIAction::ExecutePlan;
    if (action_str == "PrioritizeTask") return AIAction::PrioritizeTask;
    if (action_str == "RefactorCode") return AIAction::RefactorCode;
    if (action_str == "SuggestResearch") return AIAction::SuggestResearch;
    if (action_str == "MaximizeLearning") return AIAction::MaximizeLearning;
    return AIAction::None; // Default case
}

std::string sensor_type_to_string(SensorType type) {
    switch (type) {
        case SensorType::Keyboard: return "Keyboard";
        case SensorType::Mouse: return "Mouse";
        case SensorType::EyeTracker: return "EyeTracker";
        case SensorType::BioSensor: return "BioSensor";
        case SensorType::Microphone: return "Microphone";
        case SensorType::Camera: return "Camera";
        case SensorType::InternalAI: return "InternalAI";
        case SensorType::Network: return "Network";
        case SensorType::SystemEvent: return "SystemEvent";
        case SensorType::Display: return "Display";
        case SensorType::Battery: return "Battery";
        case SensorType::Count: return "Count";
        default: return "UnknownSensorType";
    }
}

std::string key_type_to_string(KeyType type) {
    switch (type) {
        case KeyType::Alphanumeric: return "Alphanumeric";
        case KeyType::Control: return "Control";
        case KeyType::Modifier: return "Modifier";
        case KeyType::Function: return "Function";
        case KeyType::Navigation: return "Navigation";
        case KeyType::Other: return "Other";
        default: return "UnknownKeyType";
    }
}

std::string key_event_type_to_string(KeyEventType type) {
    switch (type) {
        case KeyEventType::Press: return "Press";
        case KeyEventType::Release: return "Release";
        case KeyEventType::Hold: return "Hold";
        case KeyEventType::UndefinedEvent: return "UndefinedEvent";
        default: return "UnknownKeyEventType";
    }
}

std::string mouse_button_state_to_string(MouseButtonState state) {
    switch (state) {
        case MouseButtonState::Left: return "Left";
        case MouseButtonState::Right: return "Right";
        case MouseButtonState::Middle: return "Middle";
        case MouseButtonState::None: return "None";
        default: return "UnknownMouseButtonState";
    }
}

// === MessageQueue Implementasyonu ===
void MessageQueue::enqueue(MessageData data) {
    std::lock_guard<std::mutex> lock(mutex);
    queue.push(data);
    LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "MessageQueue: Yeni mesaj eklendi. Tür: " << static_cast<int>(data.type));
}

MessageData MessageQueue::dequeue() {
    std::lock_guard<std::mutex> lock(mutex);
    if (queue.empty()) {
        LOG_DEFAULT(CerebrumLux::LogLevel::WARNING, "MessageQueue: Kuyruk boş, boş mesaj döndürülüyor.");
        return {MessageType::Log, "Empty queue", 0, "System"}; // Boş bir mesaj döndür
    }
    MessageData data = queue.front();
    queue.pop();
    LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "MessageQueue: Mesaj çıkarıldı. Tür: " << static_cast<int>(data.type));
    return data;
}

bool MessageQueue::isEmpty() {
    std::lock_guard<std::mutex> lock(mutex);
    return queue.empty();
}

size_t MessageQueue::size() {
    std::lock_guard<std::mutex> lock(mutex);
    return queue.size();
}

} // namespace CerebrumLux