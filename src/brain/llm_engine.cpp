#include "llm_engine.h"
#include "../core/logger.h"
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
    if (!g_llama_backend_initialized) {
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
    if (ctx) { llama_free(ctx); ctx = nullptr; }
    if (model) { llama_free_model(model); model = nullptr; }
}

std::vector<llama_token> LLMEngine::tokenize(const std::string& text, bool add_bos) {
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
    if (!is_model_loaded()) return {};

    std::vector<llama_token> tokens_list = tokenize(text, true);
    
    // Modelin desteklediği max token kontrolü
    if (tokens_list.size() > n_ctx) {
        tokens_list.resize(n_ctx);
    }

    // Embedding çıkarımı için batch oluştur
    llama_batch batch = llama_batch_init(n_ctx, 0, 1);
    for (size_t i = 0; i < tokens_list.size(); i++) {
        batch.token[i] = tokens_list[i];
        batch.pos[i] = i;
        batch.n_seq_id[i] = 1;
        batch.seq_id[i][0] = 0;
        batch.logits[i] = false;
    }
    batch.n_tokens = tokens_list.size();
    // Son token için embedding/logits istiyoruz ama llama.cpp'de embedding modunda
    // tüm sekans işlendikten sonra embedding alınabilir.
    
    // Context'i temizle (önceki KV cache'i sil)
    llama_kv_cache_clear(ctx);

    if (llama_decode(ctx, batch) != 0) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "LLMEngine: Embedding decode hatası.");
        llama_batch_free(batch);
        return {};
    }

    // Embedding'i al
    float* emb = llama_get_embeddings(ctx);
    if (!emb) {
        LOG_ERROR_CERR(LogLevel::WARNING, "LLMEngine: Embedding verisi alınamadı (null). ctx_params.embeddings=true olduğundan emin olun.");
        llama_batch_free(batch);
        return {};
    }

    // Vektörü kopyala ve normalize et (Cosine Similarity için normalizasyon şart)
    int n_embd = llama_n_embd(model);
    std::vector<float> embedding(emb, emb + n_embd);

    // Normalizasyon (Euclidean Norm)
    double sum = 0.0;
    for (float f : embedding) sum += f * f;
    double norm = std::sqrt(sum);
    if (norm > 0) {
        for (float& f : embedding) f /= norm;
    }

    llama_batch_free(batch);
    return embedding;
}

std::string LLMEngine::generate(const std::string& prompt, 
                                const LLMGenerationConfig& config,
                                std::function<bool(const std::string&)> callback) {
    if (!is_model_loaded()) return "";

    std::vector<llama_token> tokens_list = tokenize(prompt, true);
    
    // KV Cache temizlenmeli çünkü embedding işlemi cache'i bozmuş olabilir.
    // Sohbet geçmişi tutuyorsak daha akıllı yönetim gerekir ama şimdilik güvenlik için temizliyoruz.
    llama_kv_cache_clear(ctx);

    llama_batch batch = llama_batch_init(2048, 0, 1); 

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
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "LLMEngine: Decode hatası.");
        llama_batch_free(batch);
        return "";
    }

    int n_cur = batch.n_tokens;
    int n_decode = 0;
    std::string full_response = "";
    std::vector<llama_token> last_n_tokens(64, 0);

    while (n_decode < config.max_tokens) {
        auto* logits = llama_get_logits_ith(ctx, batch.n_tokens - 1);
        int n_vocab = llama_n_vocab(model);
        
        std::vector<llama_token_data> candidates;
        candidates.reserve(n_vocab);
        for (int token_id = 0; token_id < n_vocab; token_id++) {
            candidates.emplace_back(llama_token_data{token_id, logits[token_id], 0.0f});
        }
        llama_token_data_array candidates_p = { candidates.data(), candidates.size(), false };

        llama_sample_repetition_penalties(ctx, &candidates_p, last_n_tokens.data(), last_n_tokens.size(), 1.25f, 0.0f, 0.0f);
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
    return full_response;
}

} // namespace CerebrumLux