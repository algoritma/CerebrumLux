#include "intent_router.h"
#include <thread>
#include <future>
#include <iostream>
#include <QCoreApplication> // QMetaObject::invokeMethod için
#include <QDebug> // qWarning için

// FastText kütüphanesini sadece burada dahil ediyoruz.
#include "../external/fasttext/include/fasttext.h"
#include "../brain/llm_engine.h" // LlamaInvoker için LLMEngine
#include "../core/logger.h" // CerebrumLux logger için
#include "../gui/DataTypes.h" // ChatResponse için

namespace CerebrumLux {

// ----------------------------------------------------
// FastTextWrapper Implementasyonu
// ----------------------------------------------------

FastTextWrapper::FastTextWrapper(const std::string &modelPath) {
    ft_model_ = std::make_unique<fasttext::FastText>();
    try {
        if (std::filesystem::exists(modelPath) && std::filesystem::file_size(modelPath) > 0) {
            ft_model_->loadModel(modelPath);
            is_model_loaded_ = true;
            LOG_DEFAULT(LogLevel::INFO, "FastTextWrapper: Model başarıyla yüklendi: " << modelPath);
        } else {
            LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "FastTextWrapper: Model bulunamadi veya boş: " << modelPath);
        }
    } catch (const std::exception& e) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "FastTextWrapper: Model yüklenirken hata: " << e.what());
    }
}

FastTextWrapper::~FastTextWrapper() {
    LOG_DEFAULT(LogLevel::INFO, "FastTextWrapper: Model boşaltılıyor.");
}

FastTextResult FastTextWrapper::classify(const std::string &text) const {
    FastTextResult r;
    r.label = "unknown";
    r.confidence = 0.0;
    
    if (!is_model_loaded_ || text.empty()) {
        LOG_DEFAULT(LogLevel::WARNING, "FastTextWrapper: Model yüklü değil veya metin boş. Sınıflandırma yapılamadı.");
        return r;
    }

    std::string normalized_text = normalizeText(text);
    if (normalized_text.empty()) {
        return r;
    }

    // FastText'in predict metodunu kullan
    std::vector<std::pair<float, std::string>> predictions;
    const int32_t k = 1; // En iyi 1 tahmin
    const float threshold = 0.0f; // Tüm tahminleri al
    
    std::stringstream ss(normalized_text);
    ft_model_->predictLine(ss, predictions, k, threshold);

    if (!predictions.empty()) {
        r.label = predictions[0].second; // '__label__<etiket>' formatında gelir
        // "__label__" kısmını temizle
        if (r.label.rfind("__label__", 0) == 0) {
            r.label = r.label.substr(9);
        }
        r.confidence = static_cast<double>(predictions[0].first);
    }
    LOG_DEFAULT(LogLevel::TRACE, "FastTextWrapper: '" << text.substr(0, std::min((size_t)20, text.length())) << "...' için FastText sınıflandırma sonucu: Label=" << r.label << ", Confidence=" << r.confidence);
    return r;
}

std::string FastTextWrapper::normalizeText(const std::string& text) const {
    std::string normalized = text;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char c){ return static_cast<unsigned char>(std::tolower(c)); });
    // Basit noktalama işaretlerini kaldırma veya başka bir temizleme
    normalized.erase(std::remove_if(normalized.begin(), normalized.end(), [](char c){ return std::ispunct(c); }), normalized.end());
    return normalized;
}

// ----------------------------------------------------
// LlamaInvoker Implementasyonu
// ----------------------------------------------------

LlamaInvoker::LlamaInvoker(LLMEngine& llm_engine_ref, const Options &opt)
    : llm_engine_(llm_engine_ref), options_(opt) {
    LOG_DEFAULT(LogLevel::INFO, "LlamaInvoker: Başlatıldı. Model yolu: " << "(LLMEngine tarafından yönetiliyor)");
}

LlamaResult LlamaInvoker::infer_sync(const std::string &prompt_text) const {
    LlamaResult res;
    res.ok = false;
    res.latency_ms = 0.0;

    auto t0 = std::chrono::steady_clock::now();

    if (!llm_engine_.is_model_loaded()) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "LlamaInvoker: LLM modeli yüklü değil, infer_sync başarısız.");
        res.text = "Üzgünüm, AI motoru hazır değil.";
        return res;
    }

    // LLMEngine'in generate metodunu çağır (const olduğu için mutable üzerinden erişebiliriz)
    LLMGenerationConfig config;
    config.max_tokens = options_.max_tokens;
    config.temperature = options_.temperature;
    config.top_p = options_.top_p;
    config.top_k = options_.top_k;
    config.repeat_penalty = options_.repeat_penalty;

    std::string llm_raw_response = llm_engine_.generate(prompt_text, config);

    auto t1 = std::chrono::steady_clock::now();
    res.latency_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

    if (!llm_raw_response.empty()) {
        res.ok = true;
        res.text = llm_raw_response;
        // Gerekçe ve önerileri response'dan parse etmek gerekebilir, şimdilik raw metni kullanıyoruz.
    } else {
        res.text = "Üzgünüm, Llama modelinden yanıt alınamadı.";
    }
    LOG_DEFAULT(LogLevel::INFO, "LlamaInvoker: Llama infer_sync tamamlandı. Latency: " << res.latency_ms << "ms");
    return res;
}

// ----------------------------------------------------
// IntentRouter Implementasyonu
// ----------------------------------------------------

IntentRouter::IntentRouter(FastTextWrapper *ft,
                         LlamaInvoker *llama,
                         const Config &cfg,
                         OnResultCb cb,
                         QObject* parent)
    : QObject(parent), ft_(ft), llama_(llama), cfg_(cfg), callback_(cb)
{
    // Varsayılan basit niyetleri ekle
    add_simple_intent("greet", "Merhaba! Sana nasıl yardımcı olabilirim?");
    add_simple_intent("ask_name", "Adım Cerebrum Lux. 'Işık Beyin' anlamına gelir.");
    add_simple_intent("ask_how", "Ben bir yapay zeka sistemiyim, dolayısıyla hislerim yok ama tüm sistemlerim %100 verimlilikle çalışıyor. Sen nasılsın?");
    add_simple_intent("system_status", "Tüm çekirdek sistemlerim stabil çalışıyor.");
    add_simple_intent("who_are_you", "Ben Cerebrum Lux. Kişisel donanımlarda çalışmak üzere tasarlanmış, mahremiyet odaklı ve yüksek performanslı bir yapay zekayım.");
    add_simple_intent("exit", "Sistemi kapatma yetkim şu an simülasyon modunda.");

    LOG_DEFAULT(LogLevel::INFO, "IntentRouter: Başlatıldı.");
    
    // QtConcurrent::run ile başlatılan asenkron Llama çağrılarını takip etmek için
    // QFutureWatcher kullanacağız, ama IntentRouter'da QFutureWatcher üyesi yok,
    // bu yüzden lambda içinden direkt emit yapacağız.
}

void IntentRouter::handle_user_input(const std::string &userId, const std::string &text) {
    std::string normalized_text = ft_->normalizeText(text); // FastText'ten alalım

    // 1. Önbellek Kontrolü
    if (cfg_.enable_cache) {
        std::lock_guard<std::mutex> lock(mtx_);
        auto it = cache_.find(normalized_text);
        if (it != cache_.end()) {
            auto tp = it->second.second;
            if (std::chrono::steady_clock::now() - tp < std::chrono::seconds(cfg_.cache_ttl_s)) {
                LOG_DEFAULT(LogLevel::INFO, "IntentRouter: Önbellekten yanıt döndürüldü. User: " << userId);                
                callback_(userId, it->second.first);
                return;
            } else {
                cache_.erase(it); // TTL dolmuş, önbellekten sil
            }
        }
    }

    // 2. FastText ile Sınıflandırma (Arka planda)
    // Bu işlem de bloklayıcı olmamalı.
    QtConcurrent::run([this, userId, text, normalized_text]() mutable {
        FastTextResult ftres = ft_->classify(text); // FastText hızlı olduğu için direkt çağrılabilir
        
        ChatResponse router_response; // IntentRouter'ın nihai yanıtı
        router_response.text = "";
        router_response.reasoning = "";

        // 3. Karar Ağacı
        bool handled_by_fasttext = false;

        // a. Doğrudan FastText Yanıtı (Yüksek güven)
        if (ftres.confidence >= cfg_.fasttext_direct_threshold) {
            std::lock_guard<std::mutex> lock(mtx_); // simple_intents'a erişim
            auto it = simple_intents_.find(ftres.label);
            if (it != simple_intents_.end()) {
                router_response.text = it->second;
                router_response.reasoning = "Refleks Katmanı (FastText Direct Reply)";
                handled_by_fasttext = true;
            } else if (!ftres.shortAnswer.empty()) { // FastText'in kendi kısa cevabı varsa
                router_response.text = ftres.shortAnswer;
                router_response.reasoning = "Refleks Katmanı (FastText Short Answer)";
                handled_by_fasttext = true;
            }
        }

        if (handled_by_fasttext) {
            if (cfg_.enable_cache) {
                std::lock_guard<std::mutex> lock(mtx_);
                cache_[normalized_text] = {router_response, std::chrono::steady_clock::now()};
            }
            callback_(userId, router_response);
            return;
        }

        // b. Llama'ya Yönlendirme (Daha düşük güven veya karmaşık niyet)
        // Eşzamanlı Llama çağrısı limitini kontrol et
        if (current_llama_calls_.load() >= cfg_.max_concurrent_llama) {
            router_response.text = "Şu an yoğunluk var, lütfen kısa süre sonra tekrar deneyin.";
            router_response.reasoning = "Llama meşgul (Concurrent Limit)";
            LOG_DEFAULT(LogLevel::WARNING, "IntentRouter: Llama eşzamanlı çağrı limitine ulaşıldı.");
            callback_(userId, router_response);
            return;
        }

        // c. Llama çağrısını başlat
        current_llama_calls_.fetch_add(1);
        LOG_DEFAULT(LogLevel::INFO, "IntentRouter: Llama çağrısı başlatılıyor. Aktif Llama çağrısı: " << current_llama_calls_.load() );

        // Llama çağrısını yeni bir thread'de asenkron olarak başlatıyoruz
        // QFutureWatcher kullanmak yerine, direkt lambda içinden sinyal emit edeceğiz
        // veya QFuture'ı tutup onLlamaCallFinished slotunda işleyeceğiz.
        // Şimdilik basitlik adına QFuture'ı ignore edip lambda içinden callback çağıralım.
        QThreadPool::globalInstance()->start([this, userId, text, normalized_text]() {
            LlamaResult lr = llama_->infer_sync(text); // LLM Engine'i çağırır (bloklar ama ayrı thread'de)
            current_llama_calls_.fetch_sub(1); // Llama çağrısı bitti
            LOG_DEFAULT(LogLevel::INFO, "IntentRouter: Llama çağrısı tamamlandı. Aktif Llama çağrısı: " << current_llama_calls_.load());

            ChatResponse llama_final_response;
            if (lr.ok) {
                llama_final_response.text = lr.text;
                llama_final_response.reasoning = "LLM ile işlendi (Latency: " + std::to_string(lr.latency_ms) + "ms)";
                llama_final_response.suggested_questions = lr.suggested_questions; // Eğer LlamaResult içinde varsa
            } else {
                llama_final_response.text = "Üzgünüm, AI modelinden yanıt alınamadı. Lütfen daha sonra tekrar deneyin.";
                llama_final_response.reasoning = "LLM çağrısı başarısız oldu veya zaman aşımına uğradı.";
            }

            if (cfg_.enable_cache) {
                std::lock_guard<std::mutex> lock(mtx_);
                cache_[normalized_text] = {llama_final_response, std::chrono::steady_clock::now()};
            }
            callback_(userId, llama_final_response);
        });
    });
}

void IntentRouter::add_simple_intent(const std::string &label, const std::string &reply) {
    std::lock_guard<std::mutex> lock(mtx_);
    simple_intents_[label] = reply;
    LOG_DEFAULT(LogLevel::INFO, "IntentRouter: Basit niyet eklendi: " << label);
}

void IntentRouter::onLlamaCallFinished() {
    // Bu slot artık kullanılmıyor, direkt lambda içinde handle ediyoruz.
    // Ancak QFutureWatcher kullanırsak burası tetiklenir.
    // Şimdilik boş kalabilir.
}

} // namespace CerebrumLux