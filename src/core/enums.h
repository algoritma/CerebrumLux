#ifndef CEREBRUM_LUX_ENUMS_H
#define CEREBRUM_LUX_ENUMS_H
#include <algorithm> // std::transform için eklendi

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
    ExecutePlan,
    PrioritizeTask, // Yeni eklenen eylem
    RefactorCode,   // Yeni eklenen eylem
    SuggestResearch,
    MaximizeLearning
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
enum class InsightType : int { // Temel tipi unsigned char'dan int'e değiştirildi
    PerformanceAnomaly,      // 0
    LearningOpportunity,     // 1
    SecurityAlert,           // 2
    EfficiencySuggestion,    // 3
    ResourceOptimization,    // 4
    BehavioralDrift,         // 5
    UserContext,             // 6
    CodeDevelopmentSuggestion, // 7 // YENİ: Kod geliştirme önerisi içgörü tipi eklendi
    None                     // 8
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

// Bilgi ve içgörüleri kategorize etmek için
enum class KnowledgeTopic : unsigned char {
    SystemPerformance,
    LearningStrategy,
    ResourceManagement,
    CyberSecurity,
    UserBehavior,
    CodeDevelopment,
    General // Genel bir kategori olarak eklendi
};

// İçgörülerin önem derecesini belirlemek için
enum class InsightSeverity : unsigned char {
    Low,
    Medium,
    High,
    Critical,
    None
};

// MouseButtonState
enum class MouseButtonState : unsigned char {
    Left,
    Right,
    Middle,
    None
};

namespace Utils {
    std::string insight_type_to_string(InsightType type);
    InsightType string_to_insight_type(const std::string& s);
    std::string action_to_string(AIAction action);
    AIAction string_to_action(const std::string& s);
} // namespace Utils

// YENİ: Desteklenen diller için enum (NLP'den buraya taşındı)
enum class Language {
    EN,
    DE,
    TR,
    UNKNOWN
};

// YENİ: Dil string'ini enum'a çeviren yardımcı fonksiyon (NLP'den buraya taşındı)
inline Language string_to_lang(const std::string& lang_str) {
    std::string lower_lang_str = lang_str;
    std::transform(lower_lang_str.begin(), lower_lang_str.end(), lower_lang_str.begin(),
                   [](unsigned char c){ return static_cast<unsigned char>(std::tolower(c)); });
    if (lower_lang_str == "en" || lower_lang_str == "english") return Language::EN;
    if (lower_lang_str == "de" || lower_lang_str == "german") return Language::DE;
    if (lower_lang_str == "tr" || lower_lang_str == "turkish") return Language::TR;
    return Language::UNKNOWN;
}

} // namespace CerebrumLux

#endif // CEREBRUM_LUX_ENUMS_H