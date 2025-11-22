#ifndef LEARNINGMODULE_H
#define LEARNINGMODULE_H

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <memory> // std::unique_ptr için

#include "KnowledgeBase.h" // CerebrumLux::KnowledgeBase'i içeriyor
#include "../communication/ai_insights_engine.h"
#include "../crypto/CryptoManager.h"
#include "UnicodeSanitizer.h" // Tam tanıma ihtiyaç duyulduğu için eklendi
#include "../swarm_vectordb/DataModels.h" // SparseQTable için
#include "../communication/natural_language_processor.h" // generate_text_embedding için
#include "StegoDetector.h"    // Tam tanıma ihtiyaç duyulduğu için eklendi
#include "WebFetcher.h" // WebFetcher için
#include "web_page_parser.h" // WebPageParser için
#include "web_search_result.h" // WebSearchResult için

#include <QObject> 
#include <QString> // QString için

namespace CerebrumLux {

// Kapsül işleme sonucunu belirten enum
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
    Busy // Örneğin, zaten mevcut bir kapsül için
};

// Kapsül işleme raporu
struct IngestReport {
    Capsule original_capsule; // Orijinal kapsül
    Capsule processed_capsule; // İşlenmiş/değiştirilmiş kapsül
    IngestResult result;      // İşlem sonucu
    std::string message;      // Detaylı mesaj
    std::string source_peer_id; // Kapsülün geldiği peer ID'si
    std::chrono::system_clock::time_point timestamp; // İşlem zamanı
    std::map<std::string, std::string> diagnostics; // Ek teşhis bilgileri

    // Varsayılan kurucu eklendi, böylece boş bir IngestReport oluşturulabilir.
    IngestReport() : result(IngestResult::UnknownError) {}
    // confidence alanı IngestReport için gereksiz, processed_capsule.confidence'dan alınabilir
};


class LearningModule : public QObject {
    Q_OBJECT 

public:
    explicit LearningModule(KnowledgeBase& kb, CerebrumLux::Crypto::CryptoManager& cryptoMan, QObject *parent = nullptr);
    ~LearningModule();

    void learnFromText(const std::string& text,
                       const std::string& source,
                       const std::string& topic,
                       float confidence = 1.0f);

    void learnFromWeb(const std::string& query);

    std::vector<Capsule> search_by_topic(const std::string& topic) const;

    void process_ai_insights(const std::vector<AIInsight>& insights);

    KnowledgeBase& getKnowledgeBase();
    const KnowledgeBase& getKnowledgeBase() const; // Const versiyonu eklendi
    
    // YENİ EKLENDİ: SparseQTable'a erişim için getter
    const CerebrumLux::SwarmVectorDB::SparseQTable& getQTable() const { return q_table; }

    // DÜZELTİLDİ: cryptoManager'a erişim için public getter eklendi.
    CerebrumLux::Crypto::CryptoManager& get_crypto_manager() const { return cryptoManager; }
    
    // Kod geliştirme önerisi geri bildirimini işler (public metot olarak eklendi)
    void processCodeSuggestionFeedback(const std::string& capsuleId, bool accepted);
    
    // Sparse Q-Table kalıcılığı için metotlar
    void save_q_table() const;
    void load_q_table();
    
    // Sparse Q-Table'ı güncellemek için metot
    void update_q_values(const std::vector<float>& current_state_embedding, CerebrumLux::AIAction action, float reward, const std::vector<float>& next_state_embedding);
    
    IngestReport ingest_envelope(const Capsule& envelope, const std::string& signature, const std::string& sender_id);

    std::vector<float> compute_embedding(const std::string& text) const;
    std::string cryptofig_encode(const std::vector<float>& cryptofig_vector) const;
    std::vector<float> cryptofig_decode_base64(const std::string& base64_cryptofig_blob) const;

signals:
    void qTableUpdated();       // Q-Table'da bir Q-değeri güncellendiğinde yayılır
    void qTableLoadCompleted(); // Q-Table LMDB'den yüklendikten sonra yayılır

    // Web çekme işleminin sonucunu bildiren sinyal
    void webFetchCompleted(const CerebrumLux::IngestReport& report);
    void knowledgeBaseUpdated(); // YENİ: KB güncellendiğinde (kapsül eklendiğinde) tetiklenir

private slots:
    // WebFetcher'dan gelen yapılandırılmış arama sonuçlarını işlemek için slot
    void onStructuredWebContentFetched(const QString& url, const std::vector<CerebrumLux::WebSearchResult>& searchResults);
    // WebFetcher'dan gelen hata sinyali için slot
    void onWebFetchError(const QString& url, const QString& error_message); 

private:
    KnowledgeBase& knowledgeBase;
    CerebrumLux::Crypto::CryptoManager& cryptoManager;
    std::unique_ptr<UnicodeSanitizer> unicodeSanitizer;
    std::unique_ptr<StegoDetector> stegoDetector;
    std::unique_ptr<WebFetcher> webFetcher;

    QObject* parentApp;
    bool webFetchInProgress = false;
    QString currentWebFetchQuery;
    CerebrumLux::SwarmVectorDB::SparseQTable q_table; // Sparse Q-Table üyesi eklendi

    bool verify_signature(const Capsule& capsule, const std::string& signature, const std::string& sender_id) const;
    Capsule decrypt_payload(const Capsule& encrypted_capsule) const;
    bool schema_validate(const Capsule& capsule) const;
    Capsule sanitize_unicode(const Capsule& capsule) const;
    bool run_steganalysis(const Capsule& capsule) const;
    bool sandbox_analysis(const Capsule& capsule) const;
    bool corroboration_check(const Capsule& capsule) const;
    void audit_log_append(const IngestReport& report) const;
    CerebrumLux::IngestReport createIngestReport(CerebrumLux::IngestResult result, const std::string& message) const;
};

} // namespace CerebrumLux

#endif // LEARNINGMODULE_H