#ifndef CEREBRUM_LUX_LLAMA_WORKER_H
#define CEREBRUM_LUX_LLAMA_WORKER_H

#include <QObject>
#include <QQueue>
#include <QMutex>
#include <QWaitCondition>
#include <QString>
#include <vector>
#include <memory>
#include <chrono>
#include <QThread> // YENİ: QThread için
#include <atomic> // std::atomic için

#include "llm_engine.h" // LLMEngine'i kullanacak
#include "../gui/DataTypes.h" // ChatResponse için
#include "../core/enums.h" // LlamaRequestType ve CryptofigAutoencoder::INPUT_DIM için (dolaylı olarak)

// CryptofigAutoencoder::INPUT_DIM tanımı için. llm_engine.h içermeliydi ama yoksa ekleyelim.
// Bu commit'te autoencoder.h'nin llm_engine.h tarafından dahil edildiğini varsayalım.
// Değilse, buraya da dahil edilebilir.
#include "autoencoder.h" // CryptofigAutoencoder::INPUT_DIM için

namespace CerebrumLux {

// Llama istek tipleri
enum class LlamaRequestType {
    INFERENCE,
    EMBEDDING,
    UNKNOWN // Varsayılan veya tanımlanmamış durumlar için
};

// LlamaWorker için bir istek yapısı
struct LlamaRequest {
    std::string userId;
    std::string prompt;
    std::vector<float> userEmbedding; // Eğer embedding de LLM tarafından hesaplanacaksa (şimdi değil)
    std::string requestId; // Yanıtı MainWindow'a eşlemek için
    CerebrumLux::LlamaRequestType requestType; // İstek tipi (inference veya embedding)
    LLMGenerationConfig config; // Generate için konfigürasyon
};

class LlamaWorker : public QObject {
    Q_OBJECT

public:
    explicit LlamaWorker(LLMEngine& llm_engine_ref, QObject* parent = nullptr);
    ~LlamaWorker();

    // Llama isteği göndermek için metod (thread-safe)
    void enqueueRequest(const LlamaRequest& request);

signals:
    // Llama inference tamamlandığında yayılacak sinyal
    void llamaResponseReady(const QString& requestId, const CerebrumLux::ChatResponse& response);
    // Embedding hazır olduğunda yayılacak sinyal
    void embeddingReady(const QString& requestId, const std::vector<float>& embedding);

public slots:
    // İş parçacığı başladığında tetiklenecek slot
    void processRequests();
    
private:
    LLMEngine& llm_engine_; // LLMEngine referansı (LlamaWorker'a dışarıdan verilir)
    QQueue<LlamaRequest> request_queue_;
    QMutex mutex_; // Kuyruk erişimi için mutex
    QWaitCondition condition_; // İstek geldiğinde iş parçacığını uyandırmak için

    std::atomic<bool> running_{true}; // İş parçacığının çalışıp çalışmadığını kontrol eder
    QThread* worker_thread_; // Kendi thread'ini yönetecek
};

} // namespace CerebrumLux

#endif // CEREBRUM_LUX_LLAMA_WORKER_H