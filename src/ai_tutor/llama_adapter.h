// llama_adapter.h
// Minimal adapter interface for LLaMA-like inference.
// Implement set_inference_fn() with your existing llama invocation.
// Thread-safe: inference function should be synchronous.

#ifndef LLAMA_ADAPTER_H
#define LLAMA_ADAPTER_H

#include <string>
#include <functional>
#include <mutex>

namespace CerebrumLux {

struct LlamaInferenceConfig {
    int max_tokens = 256;
    float temperature = 0.7f;
    bool stream_tokens = true;
};

class LlamaAdapter {
public:
    using InferFn = std::function<std::string(const std::string& prompt)>;

    // Set the sync inference function (e.g. wrapper around llama.cpp)
    static void set_inference_fn(InferFn fn);

    // Synchronous wrapper (returns model string result)
    static std::string infer_sync(const std::string &prompt);

    // Asynchronous inference with config
    std::string infer(const std::string& prompt, const LlamaInferenceConfig& cfg);

private:
    static inline InferFn infer_fn = nullptr;
    static inline std::mutex mtx;
};

} // namespace CerebrumLux


#endif // LLAMA_ADAPTER_H
