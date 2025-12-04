#include "llama_invoker.h"
#include "../core/logger.h"
#include <QDebug> // qWarning için, eğer kullanılırsa

namespace CerebrumLux {

// ----------------------------------------------------
// LlamaInvoker Implementasyonu
// ----------------------------------------------------

LlamaInvoker::LlamaInvoker(LlamaWorker& llama_worker_ref, const Options &opt)
    : llama_worker_(llama_worker_ref), options_(opt) {
    LOG_DEFAULT(LogLevel::INFO, "LlamaInvoker: Başlatıldı.");
}

void LlamaInvoker::requestInference(const std::string& userId, const std::string& prompt, const std::vector<float>& userEmbedding, const std::string& requestId, const LLMGenerationConfig& config) {
    LlamaRequest request;
    request.userId = userId;
    request.prompt = prompt;
    request.userEmbedding = userEmbedding;
    request.requestId = requestId;
    request.requestType = CerebrumLux::LlamaRequestType::INFERENCE; // İstek tipini belirt
    request.config = config;

    llama_worker_.enqueueRequest(request);
    LOG_DEFAULT(LogLevel::INFO, "LlamaInvoker: Inference isteği LlamaWorker'a iletildi. İstek ID: " << requestId);
}

void LlamaInvoker::requestEmbedding(const std::string& userId, const std::string& prompt, const std::string& requestId) {
    LlamaRequest request;
    request.userId = userId;
    request.prompt = prompt;
    request.requestId = requestId;
    request.requestType = CerebrumLux::LlamaRequestType::EMBEDDING; // İstek tipini belirt
    // Embedding istekleri için LLMGenerationConfig genellikle gerekmez, varsayılan bırakılabilir veya boş bırakılabilir.

    llama_worker_.enqueueRequest(request);
    LOG_DEFAULT(LogLevel::INFO, "LlamaInvoker: Embedding isteği LlamaWorker'a iletildi. İstek ID: " << requestId);
}

} // namespace CerebrumLux