#ifndef CEREBRUM_LUX_LLAMA_INVOKER_H
#define CEREBRUM_LUX_LLAMA_INVOKER_H

#include <string>
#include <vector>
#include "../brain/llm_engine.h" // LLMGenerationConfig için
#include "../brain/llama_worker.h" // LlamaWorker ve LlamaRequest için
#include "../core/logger.h" // LOG_DEFAULT için
#include "../core/enums.h" // LlamaRequestType için

namespace CerebrumLux {

// LlamaInvoker'ın dahili olarak döndürebileceği basit bir sonuç yapısı.
// Ancak asenkron olduğu için bu doğrudan kullanılmayacak, sinyallerle iletişim kurulacak.
struct LlamaResult {
    bool ok;
    std::string text;
    double latency_ms;
    std::string reasoning;
    std::vector<std::string> suggested_questions;
};

// LlamaWorker'a istekleri yönlendirecek arayüz sınıfı
class LlamaInvoker {
public:
    // Llama çıkarım için genel seçenekler
    struct Options {
        int n_threads = 4;
        int n_ctx = 2048; 
        int max_tokens = 512;
        int n_gpu_layers = 0;
        double temperature = 0.3;
        double top_p = 0.9;
        int top_k = 40;
        double repeat_penalty = 1.25;
        int timeout_seconds = 20; // LlamaInvoker'a özel timeout
    };

    // LlamaInvoker, LlamaWorker'ın bir referansını alır ve onun aracılığıyla istekleri kuyruğa ekler.
    explicit LlamaInvoker(LlamaWorker& llama_worker_ref, const Options &opt);
    ~LlamaInvoker() = default;

    // Llama çıkarım isteğini LlamaWorker'a iletir
    void requestInference(const std::string& userId, const std::string& prompt, const std::vector<float>& userEmbedding, const std::string& requestId, const LLMGenerationConfig& config);
    
    // Llama embedding isteğini LlamaWorker'a iletir (eğer LlamaInvoker bu sorumluluğu da alacaksa)
    void requestEmbedding(const std::string& userId, const std::string& prompt, const std::string& requestId);

private:
    LlamaWorker& llama_worker_; // LlamaWorker referansı
    Options options_; // Llama çıkarım seçenekleri
};

} // namespace CerebrumLux

#endif // CEREBRUM_LUX_LLAMA_INVOKER_H