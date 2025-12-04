#include "llama_worker.h"
#include "../core/logger.h"
#include <QDebug>
#include <QCoreApplication>
#include <chrono> // For latency calculation
#include "autoencoder.h" // CryptofigAutoencoder::INPUT_DIM için

namespace CerebrumLux {

LlamaWorker::LlamaWorker(LLMEngine& llm_engine_ref, QObject* parent) 
    : QObject(parent), llm_engine_(llm_engine_ref), worker_thread_(new QThread(this)) {
    this->moveToThread(worker_thread_); // LlamaWorker'ı kendi thread'ine taşı
    connect(worker_thread_, &QThread::started, this, &LlamaWorker::processRequests);
    connect(worker_thread_, &QThread::finished, worker_thread_, &QThread::deleteLater);
    worker_thread_->start(); // Thread'i başlat
    LOG_DEFAULT(LogLevel::INFO, "LlamaWorker: Başlatıldı ve kendi thread'ine taşındı.");
}

LlamaWorker::~LlamaWorker() {
    running_.store(false); // Thread'in durmasını işaretle
    condition_.wakeAll(); // Bekleyen tüm threadleri uyandır
    worker_thread_->quit(); // Thread'in event döngüsünü durdur
    worker_thread_->wait(); // Thread'in bitmesini bekle
    LOG_DEFAULT(LogLevel::INFO, "LlamaWorker: Sonlandırıldı ve thread temizlendi.");
}

void LlamaWorker::enqueueRequest(const LlamaRequest& request) {
    QMutexLocker locker(&mutex_);
    request_queue_.enqueue(request);
    condition_.wakeOne(); // Bir isteğin geldiğini bildir
    LOG_DEFAULT(LogLevel::TRACE, "LlamaWorker: Yeni istek kuyruğa eklendi. İstek ID: " << request.requestId);
}

void LlamaWorker::processRequests() {
    while (running_.load()) {
        LlamaRequest request;
        {
            QMutexLocker locker(&mutex_);
            if (request_queue_.isEmpty()) {
                condition_.wait(locker.mutex()); // Kuyruk boşsa bekle
                if (!running_.load()) return; // Uyandırıldı ama durma sinyali ise çık
            }
            request = request_queue_.dequeue();
        }

        if (!llm_engine_.is_model_loaded()) {
            LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "LlamaWorker: LLM modeli yüklü değil, istek işlenemedi. İstek ID: " << request.requestId);
            // Modeli yüklü değilse, hata yanıtı dön ve devam et, uygulamayı durdurma.
            if (request.requestType == LlamaRequestType::INFERENCE) {
                ChatResponse error_response;
                error_response.text = "Üzgünüm, AI motoru hazır değil. Lütfen yöneticinize başvurun.";
                error_response.reasoning = "LLM Model Not Loaded";
                emit llamaResponseReady(QString::fromStdString(request.requestId), error_response);
            } else { // EMBEDDING isteği ise
                emit embeddingReady(QString::fromStdString(request.requestId), {}); // Boş embedding dön
            }
            continue; // Bir sonraki isteğe geç
        }

        auto t0 = std::chrono::steady_clock::now();

        if (request.requestType == LlamaRequestType::EMBEDDING) {
            LOG_DEFAULT(LogLevel::INFO, "LlamaWorker: Embedding isteği işleniyor. Prompt: " << request.prompt.substr(0, std::min((size_t)50, request.prompt.length())) << "...");
            std::vector<float> embedding = llm_engine_.get_embedding(request.prompt);
            
            auto t1 = std::chrono::steady_clock::now();
            double latency_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
            LOG_DEFAULT(LogLevel::INFO, "LlamaWorker: Embedding tamamlandı. Latency: " << latency_ms << "ms. ID: " << request.requestId);

            // Embedding'i uygun boyuta küçült
            if (!embedding.empty() && embedding.size() != CerebrumLux::CryptofigAutoencoder::INPUT_DIM) {
                embedding = LLMEngine::reduce_embedding_dimension(embedding, CerebrumLux::CryptofigAutoencoder::INPUT_DIM);
            }
            emit embeddingReady(QString::fromStdString(request.requestId), embedding);
        } else { // Inference Request
            LOG_DEFAULT(LogLevel::INFO, "LlamaWorker: Inference isteği işleniyor. Prompt: " << request.prompt.substr(0, std::min((size_t)50, request.prompt.length())) << "...");
            std::string llm_raw_response_text = llm_engine_.generate(request.prompt, request.config);

            auto t1 = std::chrono::steady_clock::now();
            double latency_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

            ChatResponse response;
            response.text = llm_raw_response_text;
            response.reasoning = "LLM Inference"; // Basit bir gerekçe
            response.latency_ms = latency_ms;

            // TODO: LLM'den gelen yanıttan suggested_questions ve reasoning'i parse et
            // Şimdilik boş bırakılıyor.

            LOG_DEFAULT(LogLevel::INFO, "LlamaWorker: Inference tamamlandı. Latency: " << latency_ms << "ms. ID: " << request.requestId);
            emit llamaResponseReady(QString::fromStdString(request.requestId), response); // NOLINT(performance-unnecessary-value-param)
        }
    }
    LOG_DEFAULT(LogLevel::INFO, "LlamaWorker: İşlem döngüsü durduruldu.");
}

} // namespace CerebrumLux