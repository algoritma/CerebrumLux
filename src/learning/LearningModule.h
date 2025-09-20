#ifndef LEARNINGMODULE_H
#define LEARNINGMODULE_H

#include <string>
#include <vector> 
#include <map> // YENİ: IngestReport içinde kullanılabileceği için
#include <chrono> // YENİ: IngestReport içinde zaman damgası için

#include "KnowledgeBase.h"
#include "../communication/ai_insights_engine.h" // AIInsight için dahil edildi

// YENİ: Kapsül işleme sonucunu belirten enum
enum class IngestResult {
    Success,
    InvalidSignature,
    DecryptionFailed,
    SchemaMismatch,
    SanitizationNeeded,
    SteganographyDetected,
    SandboxFailed,
    CorroborationFailed,
    UnknownError,
    Ignored // Örneğin, zaten mevcut bir kapsül için
};

// YENİ: Kapsül işleme raporu
struct IngestReport {
    Capsule original_capsule; // Orijinal kapsül
    Capsule processed_capsule; // İşlenmiş/değiştirilmiş kapsül
    IngestResult result;      // İşlem sonucu
    std::string message;      // Detaylı mesaj
    std::string source_peer_id; // Kapsülün geldiği peer ID'si
    std::chrono::system_clock::time_point timestamp; // İşlem zamanı
    std::map<std::string, std::string> diagnostics; // Ek teşhis bilgileri
};

// YENİ: İleri bildirimler (aşağıda sınıf tanımlarını yapacağımız için)
class UnicodeSanitizer;
class StegoDetector;

class LearningModule {
public:
    // Kurucuya yeni modüller için referanslar eklenebilir, şimdilik sadece KnowledgeBase
    LearningModule(KnowledgeBase& kb);
    ~LearningModule(); // YENİ: Yıkıcı eklendi

    void learnFromText(const std::string& text,
                       const std::string& source,
                       const std::string& topic,
                       float confidence = 1.0f);

    void learnFromWeb(const std::string& query); // placeholder

    std::vector<Capsule> getCapsulesByTopic(const std::string& topic) {
        return knowledgeBase.getCapsulesByTopic(topic);
    }

    void process_ai_insights(const std::vector<AIInsight>& insights);

    KnowledgeBase& getKnowledgeBase() { return knowledgeBase; }
    const KnowledgeBase& getKnowledgeBase() const { return knowledgeBase; }

    // YENİ: Güvenli kapsül işleme pipeline'ı
    IngestReport ingest_envelope(const Capsule& envelope, const std::string& signature, const std::string& sender_id);

private:
    KnowledgeBase& knowledgeBase;

    // YENİ: Güvenli kapsül işleme için yardımcı sınıfların pointer'ları
    // LearningModule bu nesnelerin ömrünü yönetmeyebilir, dışarıdan referans olarak alabilir
    // Ancak basitleştirmek adına şimdilik doğrudan oluşturulmuş gibi düşünebiliriz
    std::unique_ptr<UnicodeSanitizer> unicodeSanitizer; // YENİ
    std::unique_ptr<StegoDetector> stegoDetector;       // YENİ

    // YENİ: ingest_envelope pipeline'ı içindeki özel metodlar (stub implementasyonlar)
    bool verify_signature(const Capsule& capsule, const std::string& signature, const std::string& sender_id);
    Capsule decrypt_payload(const Capsule& encrypted_capsule);
    bool schema_validate(const Capsule& capsule);
    Capsule sanitize_unicode(const Capsule& capsule);
    bool run_steganalysis(const Capsule& capsule);
    bool sandbox_analysis(const Capsule& capsule);
    bool corroboration_check(const Capsule& capsule);
    void audit_log_append(const IngestReport& report);
};

#endif // LEARNINGMODULE_H