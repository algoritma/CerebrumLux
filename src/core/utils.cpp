#include "utils.h"
#include <iomanip>
#include <sstream>
#include <random>
#include <thread>
#include "logger.h" // LOG_DEFAULT için

namespace CerebrumLux { // TÜM İMPLEMENTASYON BU NAMESPACE İÇİNDE OLACAK

// === SafeRNG Implementasyonu ===
SafeRNG& SafeRNG::get_instance() {
    static SafeRNG instance;
    return instance;
}

SafeRNG::SafeRNG() : generator(rd()) {
    // Kurucu
}

std::mt19937& SafeRNG::get_generator() {
    return generator;
}

// === Zamanla İlgili Yardımcı Fonksiyonlar ===
std::string get_current_timestamp_str() {
    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

long long get_current_timestamp_us() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count();
}

// === Hash Fonksiyonları ===
unsigned short hash_string(const std::string& s) { // YENİ EKLENDİ
    std::hash<std::string> hasher;
    // std::hash 64-bit bir hash döndürür, unsigned short'a sığması için kısaltıyoruz.
    // Daha robust bir hash algoritması istenirse güncellenebilir.
    return static_cast<unsigned short>(hasher(s) % 65521); // Prime sayı ile mod alarak dağıtımı artır
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
        case AIGoal::UndefinedGoal: return "UndefinedGoal"; // Yeni eklendi
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
        default: return "UnknownAction";
    }
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
        case KeyEventType::UndefinedEvent: return "UndefinedEvent"; // Yeni eklendi
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

bool MessageQueue::isEmpty() { // 'const' kaldırıldı
    std::lock_guard<std::mutex> lock(mutex);
    return queue.empty();
}

size_t MessageQueue::size() { // 'const' kaldırıldı
    std::lock_guard<std::mutex> lock(mutex);
    return queue.size();
}

} // namespace CerebrumLux
