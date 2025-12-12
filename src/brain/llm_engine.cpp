#include "llm_engine.h"
#include "../core/logger.h"
#include "../learning/UnicodeSanitizer.h" // EKLENDİ
#include <iostream>
#include <vector>
#include <cstring>
#include <algorithm>
#include <cmath> // sqrt ve pow için

namespace CerebrumLux {

// Statik backend başlatma bayrağı
static bool g_llama_backend_initialized = false;

// YENİ: Global instance tanımı
LLMEngine* LLMEngine::global_instance = nullptr;

LLMEngine::LLMEngine() {
    // Global instance'ı ayarla
    global_instance = this;
}

LLMEngine::~LLMEngine() {
    unload_model();
    if (global_instance == this) {
        global_instance = nullptr;
    }
}

bool LLMEngine::load_model(const std::string& model_path) {
    std::lock_guard<std::recursive_mutex> lock(engine_mutex); // KİLİT
    if (!g_llama_backend_initialized) { // Backend init'i mutex içinde
        llama_backend_init();
        g_llama_backend_initialized = true;
    }

    if (is_model_loaded()) {
        unload_model();
    }

    LOG_DEFAULT(LogLevel::INFO, "LLMEngine: Model yükleniyor: " << model_path);

    auto model_params = llama_model_default_params();
    model_params.n_gpu_layers = 0; 

    model = llama_load_model_from_file(model_path.c_str(), model_params);
    if (model == nullptr) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "LLMEngine: Model yüklenemedi.");
        return false;
    }

    auto ctx_params = llama_context_default_params();
    ctx_params.n_ctx = (n_ctx > 2048 || n_ctx == 0) ? 2048 : n_ctx;
    ctx_params.n_threads = n_threads;
    ctx_params.n_threads_batch = n_threads;
    
    // KRİTİK DEĞİŞİKLİK: Embedding modunu aktifleştir
    // Bu olmazsa llama_get_embeddings() null döner.
    ctx_params.embeddings = true;

    ctx = llama_new_context_with_model(model, ctx_params);
    if (ctx == nullptr) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "LLMEngine: Context oluşturulamadı.");
        llama_free_model(model);
        model = nullptr;
        return false;
    }

    LOG_DEFAULT(LogLevel::INFO, "LLMEngine: Model hazır (Chat + Embedding).");
    return true;
}

bool LLMEngine::is_model_loaded() const {
    return model != nullptr && ctx != nullptr;
}

void LLMEngine::unload_model() {
    std::lock_guard<std::recursive_mutex> lock(engine_mutex); // KİLİT
    if (ctx) { llama_free(ctx); ctx = nullptr; } // ctx null kontrolü içerde
    if (model) { llama_free_model(model); model = nullptr; } // model null kontrolü içerde
}

std::vector<llama_token> LLMEngine::tokenize(const std::string& text, bool add_bos) {
    // Bu fonksiyon zaten kilitli generate/get_embedding içinden çağrılır, burada kilide gerek yok (recursive lock hatası olur).
    int n_tokens = text.length() + add_bos; 
    std::vector<llama_token> tokens(n_tokens);
    n_tokens = llama_tokenize(model, text.c_str(), text.length(), tokens.data(), tokens.size(), add_bos, false);
    if (n_tokens < 0) {
        tokens.resize(-n_tokens);
        n_tokens = llama_tokenize(model, text.c_str(), text.length(), tokens.data(), tokens.size(), add_bos, false);
    }
    tokens.resize(n_tokens);
    return tokens;
}

std::string LLMEngine::token_to_str(llama_token token) {
    char buf[256];
    int n = llama_token_to_piece(model, token, buf, sizeof(buf), 0, true);
    if (n < 0) return "";
    return std::string(buf, n);
}

// YENİ: Embedding Üretim Fonksiyonu
std::vector<float> LLMEngine::get_embedding(const std::string& text) { 
    UnicodeSanitizer sanitizer;
    std::string sanitized_text = sanitizer.sanitize(text);
    std::lock_guard<std::recursive_mutex> lock(engine_mutex); // KİLİT
    if (!model || !ctx) return {}; // Doğrudan model ve ctx kontrolü

    std::vector<llama_token> tokens_list = tokenize(sanitized_text, true);
    
    // GÜVENLİK: Boş metin veya çok uzun metin kontrolü
    if (tokens_list.empty()) return {};
    if (tokens_list.size() > (size_t)n_ctx) tokens_list.resize(n_ctx); 

    llama_batch batch = llama_batch_init(n_ctx, 0, 1);
    for (size_t i = 0; i < tokens_list.size(); i++) {
        batch.token[i] = tokens_list[i];
        batch.pos[i] = i;
        batch.n_seq_id[i] = 1;
        batch.seq_id[i][0] = 0;
        batch.logits[i] = false;
    }
    batch.n_tokens = tokens_list.size(); 

    llama_kv_cache_clear(ctx); // KV cache'i temizliyoruz. Generate ile çakışmasın.

    if (llama_decode(ctx, batch) != 0) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "LLMEngine: Embedding decode hatası.");
        llama_batch_free(batch);
        return {};
    }

    // Embedding'i al
    float* emb = llama_get_embeddings(ctx);
    if (emb == nullptr) {
        LOG_ERROR_CERR(LogLevel::WARNING, "LLMEngine: Embedding NULL döndü. Boş vektör dönülüyor.");
        llama_batch_free(batch); 
        return {};
    }

    // Vektörü kopyala ve normalize et (Cosine Similarity için normalizasyon şart)
    int n_embd = llama_n_embd(model);
    std::vector<float> embedding(emb, emb + n_embd);

    // Normalizasyon (Euclidean Norm) (Her zaman yapmalıyız)
    double sum_sq = 0.0;
    for (float f : embedding) sum_sq += f * f;
    double norm = std::sqrt(sum_sq);
    if (norm > 1e-6) { // Sıfıra bölme hatasını engelle
        for (float& f : embedding) f /= norm;
    } else {
        LOG_DEFAULT(LogLevel::WARNING, "LLMEngine: Embedding normu sıfır, normalizasyon yapılamadı.");
    }

    llama_batch_free(batch);
    return embedding;
}

std::vector<float> CerebrumLux::LLMEngine::reduce_embedding_dimension(const std::vector<float>& original_embedding, size_t target_dim) {
    if (original_embedding.size() <= target_dim) {
        return original_embedding; // Hedef boyuttan küçük veya eşitse değişiklik yok
    }

    std::vector<float> reduced_emb(target_dim, 0.0f);
    size_t chunk_size = original_embedding.size() / target_dim;
    
    for (size_t i = 0; i < target_dim; ++i) {
        float sum = 0.0f;
        for (size_t j = 0; j < chunk_size; ++j) {
            if (i * chunk_size + j < original_embedding.size())
                sum += original_embedding[i * chunk_size + j];
        }
        reduced_emb[i] = sum / chunk_size; // Ortalama al
    }
    return reduced_emb;
}

std::string LLMEngine::generate(const std::string& prompt, 
                                const LLMGenerationConfig& config,
                                std::function<bool(const std::string&)> callback) {
    UnicodeSanitizer sanitizer;
    std::string sanitized_prompt = sanitizer.sanitize(prompt);
    std::lock_guard<std::recursive_mutex> lock(engine_mutex); // KİLİT
    if (!model || !ctx) return {};

    std::vector<llama_token> tokens_list = tokenize(sanitized_prompt, true);
    
    // --- GÜVENLİK YAMASI: BUFFER OVERFLOW ENGELLEME ---
    const int max_prompt_tokens = n_ctx - config.max_tokens;
    if (tokens_list.size() > (size_t)max_prompt_tokens) {
        LOG_DEFAULT(LogLevel::WARNING, "LLMEngine: Prompt çok uzun (" << tokens_list.size() << " token), kırpılıyor.");
        tokens_list.resize(max_prompt_tokens);
    }
    // --------------------------------------------------

    llama_kv_cache_clear(ctx); // KV cache'i temizliyoruz (Yeni prompt için)

    llama_batch batch = llama_batch_init(n_ctx, 0, 1);
    for (size_t i = 0; i < tokens_list.size(); i++) {
        batch.token[i] = tokens_list[i];
        batch.pos[i] = i;
        batch.n_seq_id[i] = 1;
        batch.seq_id[i][0] = 0;
        batch.logits[i] = false;
    }
    batch.n_tokens = tokens_list.size();
    batch.logits[batch.n_tokens - 1] = true;

    if (llama_decode(ctx, batch) != 0) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "LLMEngine: Chat Decode hatası.");
        llama_batch_free(batch);
        return "";
    }

    int n_cur = batch.n_tokens;
    int n_decode = 0;
    std::string full_response = "";
    std::vector<llama_token> last_n_tokens(64, 0); 

    while (n_decode < config.max_tokens) {
        if (n_cur >= n_ctx) break; // Context dolduysa dur

        auto* logits = llama_get_logits_ith(ctx, batch.n_tokens - 1);
        int n_vocab = llama_n_vocab(model);

        std::vector<llama_token_data> candidates;
        candidates.reserve(n_vocab);
        for (int token_id = 0; token_id < n_vocab; token_id++) {
            candidates.emplace_back(llama_token_data{token_id, logits[token_id], 0.0f});
        }
        llama_token_data_array candidates_p = { candidates.data(), candidates.size(), false };

        llama_sample_repetition_penalties(ctx, &candidates_p, last_n_tokens.data(), last_n_tokens.size(), config.repeat_penalty, 0.0f, 0.0f);
        llama_sample_top_k(ctx, &candidates_p, config.top_k, 1);
        llama_sample_top_p(ctx, &candidates_p, config.top_p, 1);
        llama_sample_temp(ctx, &candidates_p, config.temperature);
        llama_token new_token_id = llama_sample_token(ctx, &candidates_p);

        if (new_token_id == llama_token_eos(model)) break;

        std::string piece = token_to_str(new_token_id);
        full_response += piece;
        if (callback && !callback(piece)) break;

        batch.n_tokens = 0;
        batch.token[0] = new_token_id;
        batch.pos[0] = n_cur;
        batch.n_seq_id[0] = 1;
        batch.seq_id[0][0] = 0;
        batch.logits[0] = true;
        batch.n_tokens = 1;

        last_n_tokens.erase(last_n_tokens.begin());
        last_n_tokens.push_back(new_token_id);

        n_decode++;
        n_cur++;

        if (llama_decode(ctx, batch) != 0) break;
    }

    llama_batch_free(batch);
    return sanitizer.sanitize(full_response);
}

} // namespace CerebrumLux