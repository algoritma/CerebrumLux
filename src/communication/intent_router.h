#ifndef CEREBRUM_LUX_INTENT_ROUTER_H
#define CEREBRUM_LUX_INTENT_ROUTER_H

#include <string>
#include <functional>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <optional>
#include <chrono>
#include <atomic> // std::atomic için
#include <QObject> // Qt sinyalleri için
#include <QFuture> // QtConcurrent::run dönüş tipi için
#include <filesystem> // std::filesystem için
#include <QtConcurrent/QtConcurrent> // QtConcurrent::run için

#include "../external/fasttext/include/fasttext.h" // FastText'in tam tanımı için
#include "../gui/DataTypes.h" // ChatResponse'un tam tanımı için

// Forward declarations
namespace CerebrumLux {
    class NaturalLanguageProcessor; // IntentRouter için gerekli
    class LLMEngine; // LlamaInvoker için gerekli
}

namespace CerebrumLux {

// ----------------------------------------------------
// IntentRouter İçin Yardımcı Veri Yapıları
// ----------------------------------------------------

struct FastTextResult {
    std::string label;
    double confidence;
    std::string shortAnswer; // optional pre-baked answer
};

struct LlamaResult {
    bool ok;
    std::string text;
    double latency_ms;
    std::string reasoning; // LLM'den gelen gerekçe
    std::vector<std::string> suggested_questions; // LLM'den gelen öneriler
};

// Callback for results to be sent back to GUI (on GUI thread)
// userID: Kullanıcı ID'si (veya mesaj ID'si), reply: AI'ın yanıtı
using OnResultCb = std::function<void(const std::string& userId, const ChatResponse& reply)>;

// ----------------------------------------------------
// FastTextWrapper Sınıfı (FastText Modelini Yönetecek)
// ----------------------------------------------------
class FastTextWrapper {
public:
    // modelPath: FastText modelinin dosya yolu
    FastTextWrapper(const std::string &modelPath);
    ~FastTextWrapper();

    // Metni sınıflandırır ve etiket + güven + isteğe bağlı kısa yanıt döndürür
    FastTextResult classify(const std::string &text) const;

    // FastText'in sınıflandırma için ihtiyaç duyduğu ön-işleme (eğer varsa)
    std::string normalizeText(const std::string& text) const;
private:
    std::unique_ptr<fasttext::FastText> ft_model_; // FastText modelinin kendi instance'ı
    bool is_model_loaded_ = false;
};

// ----------------------------------------------------
// LlamaInvoker Sınıfı (LLM Engine Çağrılarını Yönetecek)
// ----------------------------------------------------
class LlamaInvoker {
public:
    struct Options {
        int n_threads = 4;
        int n_ctx = 2048; // LLMEngine'den geliyor
        int max_tokens = 512; // LLMGenerationConfig'ten geliyor
        int n_gpu_layers = 0;
        double temperature = 0.3; // LLMGenerationConfig'ten geliyor
        double top_p = 0.9;   // LLMGenerationConfig'ten geliyor
        int top_k = 40;     // LLMGenerationConfig'ten geliyor
        double repeat_penalty = 1.25; // LLMGenerationConfig'ten geliyor
        int timeout_seconds = 20; // LlamaInvoker'a özel timeout
    };

    // LlamaInvoker, mevcut LLMEngine'in bir referansını alır. Modelin ömrünü yönetmez.
    LlamaInvoker(LLMEngine& llm_engine_ref, const Options &opt);
    ~LlamaInvoker() = default;

    // Senkron çağrı (bloklayıcı) - işçi thread içinde kullanılır
    LlamaResult infer_sync(const std::string &prompt) const; // Options'lar constructor'dan veya LLM'den alınır

private:
    LLMEngine& llm_engine_; // Mevcut LLMEngine instance'ına referans
    Options options_;
};

// ----------------------------------------------------
// IntentRouter Sınıfı (Ana Karar Mekanizması)
// ----------------------------------------------------
class IntentRouter : public QObject { // Qt sinyalleri için QObject
public:
    struct Config {
        double fasttext_direct_threshold = 0.92; // FastText yanıtı doğrudan kabul eşiği
        double fasttext_forward_threshold = 0.70; // FastText yanıtı Llama'ya yönlendirme eşiği
        int max_concurrent_llama = 1; // Aynı anda çalışabilecek maksimum Llama çağrısı
        int llama_timeout_s = 20;     // Llama çağrısı için zaman aşımı
        bool enable_cache = true;     // Önbelleği etkinleştir
        int cache_ttl_s = 300;        // Önbellek yaşam süresi (saniye)
        std::string fasttext_model_path; // FastText model dosya yolu
    };

    // IntentRouter, FastTextWrapper ve LlamaInvoker instance'larını referans olarak alır.
    // Callback, GUI'ye sonuç döndürmek için kullanılır.
    IntentRouter(FastTextWrapper *ft,
                 LlamaInvoker *llama,
                 const Config &cfg,
                 OnResultCb cb,
                 QObject* parent = nullptr); // QObject için parent constructor'ı

    // GUI tarafından çağrılacak giriş noktası
    void handle_user_input(const std::string &userId, const std::string &text);

    // Basit niyetleri ve hazır yanıtları ekle
    void add_simple_intent(const std::string &label, const std::string &reply);

private:
    // Internal helper functions
    void process_with_fasttext(const std::string &userId, const std::string &text);
    void call_llama_async(const std::string &userId, const std::string &prompt_text);
    
    // Member variables
    FastTextWrapper* ft_;
    LlamaInvoker* llama_;
    Config cfg_;
    OnResultCb callback_; // Doğrudan fonksiyon callback'i

    std::mutex mtx_; // Cache ve simple_intents için kilit
    std::unordered_map<std::string, std::string> simple_intents_; // label -> reply
    // LRU cache: basit bir map örneği; üretimde gerçek bir LRU ile değiştirilebilir
    std::unordered_map<std::string, std::pair<ChatResponse, std::chrono::steady_clock::time_point>> cache_;

    std::atomic<int> current_llama_calls_{0}; // Eşzamanlı Llama çağrılarını kontrol eder

    // Asenkron Llama çağrılarını takip etmek için (QtConcurrent::run'ın döndürdüğü QFuture'ları tutabiliriz)
    QMap<QString, QFuture<LlamaResult>> active_llama_futures_;

private slots:
    // Llama asenkron çağrısı tamamlandığında bu slot tetiklenecek
    void onLlamaCallFinished();
};

} // namespace CerebrumLux

#endif // CEREBRUM_LUX_INTENT_ROUTER_H