#ifndef LLM_ENGINE_H
#define LLM_ENGINE_H

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <atomic>
#include <thread>

// llama.cpp başlık dosyası
#include "llama.h" 

namespace CerebrumLux {

struct LLMGenerationConfig {
    int max_tokens = 512;
    float temperature = 0.7f;
    float top_p = 0.9f;
    int top_k = 40;
    float repeat_penalty = 1.1f;
};

class LLMEngine {
public:
    LLMEngine();
    ~LLMEngine();

    // Modeli dosyadan yükler
    bool load_model(const std::string& model_path);

    // Modelin yüklü olup olmadığını kontrol eder
    bool is_model_loaded() const;

    // Verilen prompt'a göre yanıt üretir
    // callback: Her token üretildiğinde çağrılır (streaming için). 
    // Eğer callback false dönerse üretim durdurulur.
    std::string generate(const std::string& prompt, 
                         const LLMGenerationConfig& config = LLMGenerationConfig(),
                         std::function<bool(const std::string&)> callback = nullptr);

    // Model kaynaklarını serbest bırakır
    void unload_model();

private:
    llama_model* model = nullptr;
    llama_context* ctx = nullptr;
    
    // Model parametreleri
    int n_ctx = 2048; 
    int n_threads = std::thread::hardware_concurrency(); 

    // Tokenizer yardımcıları
    std::vector<llama_token> tokenize(const std::string& text, bool add_bos);
    std::string token_to_str(llama_token token);
};

} // namespace CerebrumLux

#endif // LLM_ENGINE_H