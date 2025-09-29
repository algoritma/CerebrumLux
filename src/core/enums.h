#ifndef CEREBRUM_LUX_ENUMS_H
#define CEREBRUM_LUX_ENUMS_H

#include <string>
#include <vector>

namespace CerebrumLux {

// === Log Seviyeleri ===
enum class LogLevel : unsigned char {
    TRACE,
    DEBUG,
    INFO,
    WARNING,
    ERR_CRITICAL
};

// Sensor Tipleri
enum class SensorType : unsigned char {
    Keyboard,
    Mouse,
    EyeTracker,
    BioSensor,
    Microphone,
    Camera,
    InternalAI,
    Network,
    SystemEvent,
    Display,
    Battery,
    Count // YENİ EKLENDİ: Enum'daki toplam eleman sayısı için
};

// AI Eylem Türleri
enum class AIAction : unsigned char {
    None,
    RespondToUser,
    SuggestSelfImprovement,
    AdjustLearningRate,
    RequestMoreData,
    QuarantineCapsule,
    InitiateHandshake,
    PerformWebSearch,
    UpdateKnowledgeBase,
    MonitorPerformance,
    CalibrateSensors,
    ExecutePlan
};

// Niyet Türleri
enum class UserIntent : unsigned char {
    Undefined,
    Question,
    Command,
    Statement,
    FeedbackPositive,
    FeedbackNegative,
    Greeting,
    Farewell,
    RequestInformation,
    ExpressEmotion,
    Confirm,
    Deny,
    Elaborate,
    Clarify,
    CorrectError,
    InquireCapability,
    ShowStatus,
    ExplainConcept,
    FastTyping,
    Programming,      // Eklendi
    Gaming,           // Eklendi
    MediaConsumption, // Eklendi
    CreativeWork,     // Eklendi
    Research,         // Eklendi
    Communication,    // Eklendi
    Editing,          // Eklendi
    Unknown           // YENİ EKLENDİ (response_engine.cpp'deki 'Unknown' hatası için)
};

// Soyut Durumlar
enum class AbstractState : unsigned char {
    Idle,
    AwaitingInput,
    ProcessingInput,
    Responding,
    Learning,
    ExecutingTask,
    Error,
    Calibrating,
    SelfReflecting,
    SuspiciousActivity,
    NormalOperation,
    SeekingInformation,
    PowerSaving,
    FaultyHardware,
    Distracted,
    Focused,
    HighProductivity,
    LowProductivity,
    Debugging,
    CreativeFlow,
    PassiveConsumption,
    SocialInteraction,
    Undefined // YENİ EKLENDİ: Tanımsız veya özel bir durum için
};

// İçgörü Türleri
enum class InsightType : unsigned char {
    PerformanceAnomaly,
    LearningOpportunity,
    SecurityAlert,
    EfficiencySuggestion,
    ResourceOptimization,
    BehavioralDrift,
    UserContext,
    CodeDevelopmentSuggestion, // YENİ: Kod geliştirme önerisi içgörü tipi eklendi
    None
};

// Aciliyet Seviyeleri
enum class UrgencyLevel : unsigned char {
    Low,
    Medium,
    High,
    Critical,
    None
};

// Bilgi Kapsülü Bağlam Türleri
enum class CapsuleContextType : unsigned char {
    General,
    Technical,
    UserSpecific,
    Historical,
    Realtime
};

// AIGoal
enum class AIGoal : unsigned char {
    OptimizeProductivity,
    MinimizeErrors,
    MaximizeLearning,
    EnsureSecurity,
    MaintainUserSatisfaction,
    ConserveResources,
    ExploreNewKnowledge,
    UndefinedGoal
};

// KeyType
enum class KeyType : unsigned char {
    Alphanumeric,
    Control,
    Modifier,
    Function,
    Navigation,
    Other
};

// KeyEventType
enum class KeyEventType : unsigned char {
    Press,
    Release,
    Hold,
    UndefinedEvent
};

// MouseButtonState
enum class MouseButtonState : unsigned char {
    Left,
    Right,
    Middle,
    None
};

} // namespace CerebrumLux

#endif // CEREBRUM_LUX_ENUMS_H