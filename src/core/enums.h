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

// LogLevel için to_string eklendi
inline std::string to_string(LogLevel level) {
    switch (level) {
        case LogLevel::TRACE: return "TRACE";
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERR_CRITICAL: return "ERR_CRITICAL";
    }
    return "UNKNOWN";
}


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
    None = 0,
    Respond,
    AskClarification,
    Teach,
    Ignore,
    LogOnly,
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
    MaximizeLearning, // Yeni eklenen eylem
    TutorRespond,
    OptimizeProductivity,
    MinimizeErrors,
    EnsureSecurity,
    MaintainUserSatisfaction,
    ConserveResources,
    ExploreNewKnowledge,
    AdaptToChange,
    PredictFutureTrends,
    OptimizeResourceAllocation,
    GenerateReports,
    Evaluate
};

// action_to_string kaldırıldı, to_string(AIAction) kullanılmalı

inline std::string to_string(AIAction action) {
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
        case AIAction::TutorRespond: return "TutorRespond";
        case AIAction::OptimizeProductivity: return "OptimizeProductivity";
        case AIAction::MinimizeErrors: return "MinimizeErrors";
        case AIAction::EnsureSecurity: return "EnsureSecurity";
        case AIAction::MaintainUserSatisfaction: return "MaintainUserSatisfaction";
        case AIAction::ConserveResources: return "ConserveResources";
        case AIAction::ExploreNewKnowledge: return "ExploreNewKnowledge";
        case AIAction::AdaptToChange: return "AdaptToChange";
        case AIAction::PredictFutureTrends: return "PredictFutureTrends";
        case AIAction::OptimizeResourceAllocation: return "OptimizeResourceAllocation";
        case AIAction::GenerateReports: return "GenerateReports";
        case AIAction::Evaluate: return "Evaluate";
    }
    return "Unknown"; // Should not happen
}

inline AIAction string_to_action(const std::string& s) {
    if (s == "None") return AIAction::None;
    if (s == "RespondToUser") return AIAction::RespondToUser;
    if (s == "SuggestSelfImprovement") return AIAction::SuggestSelfImprovement;
    if (s == "AdjustLearningRate") return AIAction::AdjustLearningRate;
    if (s == "RequestMoreData") return AIAction::RequestMoreData;
    if (s == "QuarantineCapsule") return AIAction::QuarantineCapsule;
    if (s == "InitiateHandshake") return AIAction::InitiateHandshake;
    if (s == "PerformWebSearch") return AIAction::PerformWebSearch;
    if (s == "UpdateKnowledgeBase") return AIAction::UpdateKnowledgeBase;
    if (s == "MonitorPerformance") return AIAction::MonitorPerformance;
    if (s == "CalibrateSensors") return AIAction::CalibrateSensors;
    if (s == "ExecutePlan") return AIAction::ExecutePlan;
    if (s == "PrioritizeTask") return AIAction::PrioritizeTask;
    if (s == "RefactorCode") return AIAction::RefactorCode;
    if (s == "SuggestResearch") return AIAction::SuggestResearch;
    if (s == "MaximizeLearning") return AIAction::MaximizeLearning;
    if (s == "TutorRespond") return AIAction::TutorRespond;
    if (s == "OptimizeProductivity") return AIAction::OptimizeProductivity;
    if (s == "MinimizeErrors") return AIAction::MinimizeErrors;
    if (s == "EnsureSecurity") return AIAction::EnsureSecurity;
    if (s == "MaintainUserSatisfaction") return AIAction::MaintainUserSatisfaction;
    if (s == "ConserveResources") return AIAction::ConserveResources;
    if (s == "ExploreNewKnowledge") return AIAction::ExploreNewKnowledge;
    if (s == "AdaptToChange") return AIAction::AdaptToChange;
    if (s == "PredictFutureTrends") return AIAction::PredictFutureTrends;
    if (s == "OptimizeResourceAllocation") return AIAction::OptimizeResourceAllocation;
    if (s == "GenerateReports") return AIAction::GenerateReports;
    if (s == "Evaluate") return AIAction::Evaluate;
    return AIAction::None; // Varsayılan veya bilinmeyen eylem
}

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

inline std::string to_string(UserIntent intent) {
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
    }
    return "Unknown"; // Should not happen
}

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

inline std::string to_string(InsightType type) {
    switch (type) {
        case InsightType::PerformanceAnomaly: return "PerformanceAnomaly";
        case InsightType::LearningOpportunity: return "LearningOpportunity";
        case InsightType::SecurityAlert: return "SecurityAlert";
        case InsightType::EfficiencySuggestion: return "EfficiencySuggestion";
        case InsightType::ResourceOptimization: return "ResourceOptimization";
        case InsightType::BehavioralDrift: return "BehavioralDrift";
        case InsightType::UserContext: return "UserContext";
        case InsightType::CodeDevelopmentSuggestion: return "CodeDevelopmentSuggestion";
        case InsightType::None: return "None";
    }
    return "None"; // Should not happen
}

inline InsightType string_to_insight_type(const std::string& s) {
    if (s == "PerformanceAnomaly") return InsightType::PerformanceAnomaly;
    if (s == "LearningOpportunity") return InsightType::LearningOpportunity;
    if (s == "SecurityAlert") return InsightType::SecurityAlert;
    if (s == "EfficiencySuggestion") return InsightType::EfficiencySuggestion;
    if (s == "ResourceOptimization") return InsightType::ResourceOptimization;
    if (s == "BehavioralDrift") return InsightType::BehavioralDrift;
    if (s == "UserContext") return InsightType::UserContext;
    if (s == "CodeDevelopmentSuggestion") return InsightType::CodeDevelopmentSuggestion;
    return InsightType::None;
}

// Aciliyet Seviyeleri
enum class UrgencyLevel : unsigned char {
    Low,
    Medium,
    High,
    Critical,
    None
};

inline std::string to_string(UrgencyLevel level) {
    switch (level) {
        case UrgencyLevel::Low: return "Low";
        case UrgencyLevel::Medium: return "Medium";
        case UrgencyLevel::High: return "High";
        case UrgencyLevel::Critical: return "Critical";
        case UrgencyLevel::None: return "None";
    }
    return "None"; // Should not happen
}

inline UrgencyLevel string_to_urgency_level(const std::string& s) {
    if (s == "Low") return UrgencyLevel::Low;
    if (s == "Medium") return UrgencyLevel::Medium;
    if (s == "High") return UrgencyLevel::High;
    if (s == "Critical") return UrgencyLevel::Critical;
    return UrgencyLevel::None;
}

// Bilgi Kapsülü Bağlam Türleri
enum class CapsuleContextType : unsigned char {
    General,
    Technical,
    UserSpecific,
    Historical,
    Realtime
};

inline std::string to_string(CapsuleContextType type) {
    switch (type) {
        case CapsuleContextType::General: return "General";
        case CapsuleContextType::Technical: return "Technical";
        case CapsuleContextType::UserSpecific: return "UserSpecific";
        case CapsuleContextType::Historical: return "Historical";
        case CapsuleContextType::Realtime: return "Realtime";
    }
    return "General"; // Should not happen
}

inline CapsuleContextType string_to_capsule_context_type(const std::string& s) {
    if (s == "General") return CapsuleContextType::General;
    if (s == "Technical") return CapsuleContextType::Technical;
    if (s == "UserSpecific") return CapsuleContextType::UserSpecific;
    if (s == "Historical") return CapsuleContextType::Historical;
    if (s == "Realtime") return CapsuleContextType::Realtime;
    return CapsuleContextType::General;
}

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

inline std::string to_string(AIGoal goal) {
    switch (goal) {
        case AIGoal::OptimizeProductivity: return "OptimizeProductivity";
        case AIGoal::MinimizeErrors: return "MinimizeErrors";
        case AIGoal::MaximizeLearning: return "MaximizeLearning";
        case AIGoal::EnsureSecurity: return "EnsureSecurity";
        case AIGoal::MaintainUserSatisfaction: return "MaintainUserSatisfaction";
        case AIGoal::ConserveResources: return "ConserveResources";
        case AIGoal::ExploreNewKnowledge: return "ExploreNewKnowledge";
        case AIGoal::UndefinedGoal: return "UndefinedGoal";
    }
    return "UndefinedGoal"; // Should not happen
}

inline AIGoal string_to_ai_goal(const std::string& s) {
    if (s == "OptimizeProductivity") return AIGoal::OptimizeProductivity;
    if (s == "MinimizeErrors") return AIGoal::MinimizeErrors;
    if (s == "MaximizeLearning") return AIGoal::MaximizeLearning;
    if (s == "EnsureSecurity") return AIGoal::EnsureSecurity;
    if (s == "MaintainUserSatisfaction") return AIGoal::MaintainUserSatisfaction;
    if (s == "ConserveResources") return AIGoal::ConserveResources;
    if (s == "ExploreNewKnowledge") return AIGoal::ExploreNewKnowledge;
    return AIGoal::UndefinedGoal;
}

// KeyType
enum class KeyType : unsigned char {
    Alphanumeric,
    Control,
    Modifier,
    Function,
    Navigation,
    Other
};

inline std::string to_string(KeyType type) {
    switch (type) {
        case KeyType::Alphanumeric: return "Alphanumeric";
        case KeyType::Control: return "Control";
        case KeyType::Modifier: return "Modifier";
        case KeyType::Function: return "Function";
        case KeyType::Navigation: return "Navigation";
        case KeyType::Other: return "Other";
    }
    return "Other"; // Should not happen
}

inline KeyType string_to_key_type(const std::string& s) {
    if (s == "Alphanumeric") return KeyType::Alphanumeric;
    if (s == "Control") return KeyType::Control;
    if (s == "Modifier") return KeyType::Modifier;
    if (s == "Function") return KeyType::Function;
    if (s == "Navigation") return KeyType::Navigation;
    return KeyType::Other;
}

// KeyEventType
enum class KeyEventType : unsigned char {
    Press,
    Release,
    Hold,
    UndefinedEvent
};

inline std::string to_string(KeyEventType type) {
    switch (type) {
        case KeyEventType::Press: return "Press";
        case KeyEventType::Release: return "Release";
        case KeyEventType::Hold: return "Hold";
        case KeyEventType::UndefinedEvent: return "UndefinedEvent";
    }
    return "UndefinedEvent"; // Should not happen
}

inline KeyEventType string_to_key_event_type(const std::string& s) {
    if (s == "Press") return KeyEventType::Press;
    if (s == "Release") return KeyEventType::Release;
    if (s == "Hold") return KeyEventType::Hold;
    return KeyEventType::UndefinedEvent;
}

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

inline std::string to_string(KnowledgeTopic topic) {
    switch (topic) {
        case KnowledgeTopic::SystemPerformance: return "SystemPerformance";
        case KnowledgeTopic::LearningStrategy: return "LearningStrategy";
        case KnowledgeTopic::ResourceManagement: return "ResourceManagement";
        case KnowledgeTopic::CyberSecurity: return "CyberSecurity";
        case KnowledgeTopic::UserBehavior: return "UserBehavior";
        case KnowledgeTopic::CodeDevelopment: return "CodeDevelopment";
        case KnowledgeTopic::General: return "General";
    }
    return "General"; // Should not happen
}

inline KnowledgeTopic string_to_knowledge_topic(const std::string& s) {
    if (s == "SystemPerformance") return KnowledgeTopic::SystemPerformance;
    if (s == "LearningStrategy") return KnowledgeTopic::LearningStrategy;
    if (s == "ResourceManagement") return KnowledgeTopic::ResourceManagement;
    if (s == "CyberSecurity") return KnowledgeTopic::CyberSecurity;
    if (s == "UserBehavior") return KnowledgeTopic::UserBehavior;
    if (s == "CodeDevelopment") return KnowledgeTopic::CodeDevelopment;
    return KnowledgeTopic::General;
}

// İçgörülerin önem derecesini belirlemek için
enum class InsightSeverity : unsigned char {
    Low,
    Medium,
    High,
    Critical,
    None
};

inline std::string to_string(InsightSeverity severity) {
    switch (severity) {
        case InsightSeverity::Low: return "Low";
        case InsightSeverity::Medium: return "Medium";
        case InsightSeverity::High: return "High";
        case InsightSeverity::Critical: return "Critical";
        case InsightSeverity::None: return "None";
    }
    return "None"; // Should not happen
}

inline InsightSeverity string_to_insight_severity(const std::string& s) {
    if (s == "Low") return InsightSeverity::Low;
    if (s == "Medium") return InsightSeverity::Medium;
    if (s == "High") return InsightSeverity::High;
    if (s == "Critical") return InsightSeverity::Critical;
    return InsightSeverity::None;
}

// MouseButtonState
enum class MouseButtonState : unsigned char {
    Left,
    Right,
    Middle,
    None
};

inline std::string to_string(MouseButtonState state) {
    switch (state) {
        case MouseButtonState::Left: return "Left";
        case MouseButtonState::Right: return "Right";
        case MouseButtonState::Middle: return "Middle";
        case MouseButtonState::None: return "None";
    }
    return "None"; // Should not happen
}

inline MouseButtonState string_to_mouse_button_state(const std::string& s) {
    if (s == "Left") return MouseButtonState::Left;
    if (s == "Right") return MouseButtonState::Right;
    if (s == "Middle") return MouseButtonState::Middle;
    return MouseButtonState::None;
}


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