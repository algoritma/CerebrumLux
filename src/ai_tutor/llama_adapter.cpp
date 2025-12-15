// llama_adapter.cpp
#include "llama_adapter.h"
#include <stdexcept>

namespace CerebrumLux {

void LlamaAdapter::set_inference_fn(InferFn fn) {
    std::lock_guard lk(mtx);
    infer_fn = fn;
}

std::string LlamaAdapter::infer_sync(const std::string &prompt) {
    std::lock_guard lk(mtx);
    if (!infer_fn) throw std::runtime_error("LlamaAdapter::infer_fn not set");
    return infer_fn(prompt);
}

std::string LlamaAdapter::infer(const std::string& prompt, const LlamaInferenceConfig& cfg) {
    (void)cfg; // şimdilik config sync wrapper’da kullanılmıyor
    return infer_sync(prompt);
}

} // namespace CerebrumLux