#ifndef CEREBRUM_LUX_ENUMS_H
#define CEREBRUM_LUX_ENUMS_H

// Bu dosyada tüm enum tanımları yer alacak:
// KeyType, KeyEventType, UserIntent, AIAction, SensorType, AbstractState, AIGoal, LogLevel

// İleri bildirimler (gerekirse)
// extern LogLevel g_current_log_level; // Global değişken, utils.h/cpp'ye taşınacak

// İleri bildirimler (gerekirse, bu dosyanın dışındaki sınıflar için)
// Bu dosya diğer hiçbir şeyi doğrudan #include etmeyecek, temel enumları sağlayacak.


//İçerik Notu: cerebrum_lux_core.h dosyasındaki tüm enum class tanımlarını (KeyType'tan AIGoal'e kadar) buraya taşıyın. g_current_log_level bildirimi buradan kaldırılacak, utils.h'e geçecek.

// Enum tanımları
enum class KeyType : unsigned char;
enum class KeyEventType : unsigned char;
enum class UserIntent : unsigned char;
enum class AIAction : unsigned char;
enum class SensorType : unsigned char;
enum class AbstractState : unsigned char;
enum class AIGoal : unsigned char;
enum class LogLevel : unsigned char;

// Basilan tusun tipi
enum class KeyType : unsigned char {
    Alphanumeric, // Harf veya sayilar
    Whitespace,   // Bosluk, Sekme
    Control,      // Ctrl, Alt, Shift, Fn
    Modifier,     // Super/Windows/Command tuslari
    Navigation,   // Ok tuslari, Home, End
    Function,     // F1-F12
    Backspace,    // Geri tusu
    Enter,        // Enter tusu
    Other         // Diger ozel karakterler
};

// Tus olayi tipi (basma/birakma)
enum class KeyEventType : unsigned char {
    Press,
    Release
};

// Tahmini kullanici niyetleri
enum class UserIntent : unsigned char {
    None,         // Hiçbir niyetin belirlenmediği varsayılan durum için
    Unknown,      // Bilinmiyor
    FastTyping,   // Hızlı Yazma Modu
    Editing,      // Düzenleme Modu
    IdleThinking, // Düşünme/Ara Verme Modu
    
    // YENİ NİYETLER
    Programming,      // Yazılım geliştirme
    Gaming,           // Oyun oynama
    MediaConsumption, // Medya tüketimi (video izleme, müzik dinleme)
    CreativeWork,     // Yaratıcı çalışma (grafik tasarım, müzik prodüksiyonu)
    Research,         // Araştırma yapma (tarayıcıda yoğun gezinme, okuma)
    Communication,    // İletişim (mesajlaşma, e-posta)

    // Eksik olanlar eklendi
    VideoEditing,     // Video düzenleme
    Browsing,         // İnternette gezinme
    Reading,          // Okuma
    GeneralInquiry,   // Genel sorgulama

    Count             // Enum büyüklüğünü saymak için (yardımcı üye)
};

// AI'in yapabileceği veya önerebileceği eylemler
enum class AIAction : unsigned char {
    None,
    DisableSpellCheck,
    EnableCustomDictionary,
    ShowUndoHistory,
    CompareVersions,
    DimScreen,
    MuteNotifications,
    LaunchApplication, 
    OpenFile,          
    SetReminder,       
    SimulateOSAction,  
    SuggestBreak,       // Ara vermeyi öner
    OptimizeForGaming,  // Oyun performansı için optimize et
    EnableFocusMode,    // Odaklanma modunu etkinleştir
    AdjustAudioVolume,  // Ses seviyesini ayarla
    OpenDocumentation,  // Dokümantasyon aç (Programlama için)
    SuggestSelfImprovement, // AI'nın kendini geliştirme önerisi

    Count             // Enum büyüklüğünü saymak için
};

// Yeni sensör tipleri
enum class SensorType : unsigned char {
    None,             // YENİ: Hiçbir sensör tipinin belirlenmediği varsayılan durum için
    Keyboard,
    Mouse,
    Display,
    Battery,
    Network,
    Application,
    Microphone,
    Camera,
    GPS,
    HeartRate,
    AmbientLight,
    Temperature,
    Humidity,
    Pressure,
    Proximity,
    Accelerometer,
    Gyroscope,
    Magnetometer,
    Barometer,
    UVLight,
    Infrared,
    Gesture,
    Speech,
    EyeTracking,
    FacialExpression,
    BodyPosture,
    BrainActivity,
    EnvironmentalAudio,
    Touch,
    ProximityGesture,
    Haptic,
    Force,
    Chemical,
    HeartSound,
    BloodPressure,
    Glucose,
    Sleep,
    Stress,
    Hydration,
    Activity,
    Location,
    ObjectDetection,
    PoseEstimation,
    EmotionRecognition,
    SpeechRecognition,
    ContextAwareness,
    CognitiveLoad,
    EnvironmentalNoise,
    AirQuality,
    UVIndex,
    HeartRateVariability,
    GalvanicSkinResponse,
    BloodOxygen,
    RespirationRate,
    BloodAlcohol,
    GPSAccuracy,
    Compass,
    Altimeter,
    GeomagneticRotation,
    RelativeHumidity,
    AbsoluteHumidity,
    DewPoint,
    WindSpeed,
    WindDirection,
    Rainfall,
    Snowfall,
    CloudCover,
    Visibility,
    WaterTemperature,
    WaterPressure,
    WaterFlow,
    SoilMoisture,
    SoilTemperature,
    LeafWetness,
    SolarRadiation,
    Chlorophyll,
    PH,
    ElectricalConductivity,
    DissolvedOxygen,
    Turbidity,
    Salinity,
    ORP,
    IonSelective,
    Gas,
    Smoke,
    Flame,
    Motion,
    Vibration,
    Acoustic,
    Ultrasonic,
    InfraredProximity,
    Radar,
    Lidar,
    GeigerCounter,
    Seismic,
    Weight,
    Load,
    Torque,
    Flow,
    Level,
    Position,
    Displacement,
    Velocity,
    Acceleration,
    Angle,
    Distance,
    LightIntensity,
    Color,
    InfraredTemperature,
    MicrophoneArray,
    CameraArray,
    LidarArray,
    RadarArray,
    EEG,
    ECG,
    EMG,
    EOG,
    GSR,
    PPG,
    SPO2,
    BloodPressureCuff,
    GlucoseMeter,
    SleepMonitor,
    StressMonitor,
    HydrationMonitor,
    ActivityTracker,
    LocationTracker,
    ObjectRecognition,
    PoseTracking,
    EmotionDetection,
    SpeechDetection,
    ContextRecognition,
    CognitiveLoadMonitor,
    EnvironmentalNoiseMonitor,
    AirQualityMonitor,
    UVIndexMonitor,
    HeartRateVariabilityMonitor,
    GalvanicSkinResponseMonitor,
    BloodOxygenMonitor,
    RespirationRateMonitor,
    BloodAlcoholMonitor,
    GPSReceiver,
    CompassSensor,
    AltimeterSensor,
    GeomagneticRotationSensor,
    RelativeHumiditySensor,
    AbsoluteHumiditySensor,
    DewPointSensor,
    WindSpeedSensor,
    WindDirectionSensor,
    RainfallSensor,
    SnowfallSensor,
    CloudCoverSensor,
    VisibilitySensor,
    WaterTemperatureSensor,
    WaterPressureSensor,
    WaterFlowSensor,
    SoilMoistureSensor,
    SoilTemperatureSensor,
    LeafWetnessSensor,
    SolarRadiationSensor,
    ChlorophyllSensor,
    PHSensor,
    ElectricalConductivitySensor,
    DissolvedOxygenSensor,
    TurbiditySensor,
    SalinitySensor,
    ORPSensor,
    IonSelectiveSensor,
    GasSensor,
    SmokeSensor,
    FlameSensor,
    MotionSensor,
    VibrationSensor,
    AcousticSensor,
    UltrasonicSensor,
    InfraredProximitySensor,
    RadarSensor,
    LidarSensor,
    GeigerCounterSensor,
    SeismicSensor,
    WeightSensor,
    LoadSensor,
    TorqueSensor,
    FlowSensor,
    LevelSensor,
    PositionSensor,
    DisplacementSensor,
    VelocitySensor,
    AccelerationSensor,
    AngleSensor,
    DistanceSensor,
    LightIntensitySensor,
    ColorSensor,
    InfraredTemperatureSensor,

    Count             // Enum büyüklüğünü saymak için
};

// Daha yüksek seviyeli, soyut durumlar
enum class AbstractState : unsigned char {
    None,
    HighProductivity,
    LowProductivity,
    Focused,
    Distracted,
    PowerSaving,
    NormalOperation,
    
    // YENİ SOYUT DURUMLAR
    CreativeFlow,     // Yaratıcı bir "akış" durumunda olma
    Debugging,        // Hata ayıklama modunda olma (yoğun kontrol tuşu, hızlı geri alma)
    PassiveConsumption, // Pasif medya tüketimi (sadece izleme/dinleme)
    HardwareAnomaly,  // Donanım anormalliği (pil, ağ, disk performansı sorunları)
    SeekingInformation, // Bilgi arayışı (tarayıcı, arama motorları)
    SocialInteraction,  // Sosyal etkileşim (mesajlaşma, sosyal medya)

    // Eksik olanlar eklendi
    HighPerformance,  // Yüksek performans modu
    FaultyHardware,   // Arızalı donanım durumu

    Count             // Enum büyüklüğünü saymak için
};

// AI'ın ulaşmaya çalışacağı hedefler
enum class AIGoal : unsigned char {
    None,
    OptimizeProductivity,
    MaximizeBatteryLife,
    ReduceDistractions,
    // Gelecekte eklenecek diger hedefler
    EnhanceCreativity,    // Yaratıcılığı artırma
    ImproveGamingExperience, // Oyun deneyimini iyileştirme
    FacilitateResearch,   // Araştırmayı kolaylaştırma
    SelfImprovement,      // AI'nın kendi kendini geliştirme hedefi

    Count                 // Enum büyüklüğünü saymak için
};

// Raporlama seviyeleri
enum class LogLevel : unsigned char {
    SILENT,   // Hiçbir çıktı
    ERR_CRITICAL,    // Sadece hatalar (ERROR makro çakışmasını önlemek için yeniden adlandırıldı)
    WARNING,  // Hatalar ve uyarılar
    INFO,     // Genel bilgi mesajları (varsayılan)
    DEBUG,    // Detaylı hata ayıklama bilgisi
    TRACE     // Çok detaylı, her adımı gösteren çıktılar
};


// LearningRateAdjustmentMessage 
enum class MessageType {
    Unknown,
    LearningRateAdjustment
};

struct MessageData {
    MessageType type;
    float learningRate;
};

#endif // CEREBRUM_LUX_ENUMS_H