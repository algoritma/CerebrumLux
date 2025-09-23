#include "LearningModule.h" 
#include <iostream>
#include <algorithm> 
#include "../core/logger.h" 
#include "../learning/WebFetcher.h" 
#include "../core/utils.h" 
#include "../external/nlohmann/json.hpp" 

// OpenSSL Başlıkları (Bu dosya içinde OpenSSL API'leri doğrudan kullanılacak)
#include <openssl/crypto.h> 
#include <openssl/ssl.h>    
#include <openssl/evp.h>    
#include <openssl/rand.h>   
#include <openssl/err.h>    
#include <openssl/bio.h>    
#include <openssl/buffer.h> 
// #include <openssl/ed25519.h> // Yorum satırı kalıyor

// YENİ: UnicodeSanitizer ve StegoDetector başlık dosyaları
#include "UnicodeSanitizer.h"
#include "StegoDetector.h"

// Kapsül ID sayacını string ID'lere uyarlamak için (isteğe bağlı)
static unsigned int s_learning_module_capsule_id_counter = 0;

// ================================================================
// base64_encode_internal ve base64_decode_internal implementasyonları
// ================================================================

std::string LearningModule::base64_encode_internal(const std::string& in) const {
    BIO *b64, *bmem;
    BUF_MEM *bptr;

    b64 = BIO_new(BIO_f_base64());
    bmem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bmem);
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL); 
    BIO_write(b64, in.data(), static_cast<int>(in.size()));
    BIO_flush(b64);

    char *data;
    long len = BIO_get_mem_data(bmem, &data); 
    std::string out(data, len);

    BIO_free_all(b64);
    return out;
}

std::string LearningModule::base64_decode_internal(const std::string& in) const {
    BIO *b64, *bmem;
    char* buffer = nullptr;
    size_t length = 0;

    b64 = BIO_new(BIO_f_base64());
    bmem = BIO_new_mem_buf(in.data(), static_cast<int>(in.size())); 
    b64 = BIO_push(b64, bmem);
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);

    length = in.length();
    buffer = (char*)OPENSSL_malloc(length + 1); 
    if (!buffer) {
        LOG_DEFAULT(LogLevel::ERR_CRITICAL, "Base64 decode için bellek ayrılamadı.");
        return "";
    }
    
    int decoded_len = BIO_read(b64, buffer, static_cast<int>(length));
    if (decoded_len < 0) {
        LOG_DEFAULT(LogLevel::ERR_CRITICAL, "Base64 kod çözme hatası.");
        OPENSSL_free(buffer);
        BIO_free_all(b64);
        return "";
    }
    buffer[decoded_len] = '\0'; 

    std::string out(buffer, decoded_len);
    OPENSSL_free(buffer);
    BIO_free_all(b64);
    return out;
}

// ================================================================
// Yeni public virtual Base64 string metotlarının implementasyonu
// ================================================================

std::string LearningModule::base64_encode_string(const std::string& data) const {
    return this->base64_encode_internal(data);
}

std::string LearningModule::base64_decode_string(const std::string& data) const {
    return this->base64_decode_internal(data);
}

// ================================================================

// Kurucu
LearningModule::LearningModule(KnowledgeBase& kb) 
    : knowledgeBase(kb),
      unicodeSanitizer(std::make_unique<UnicodeSanitizer>()), 
      stegoDetector(std::make_unique<StegoDetector>())      
{
    LOG_DEFAULT(LogLevel::INFO, "LearningModule: Initialized.");
}

// Yıkıcı
LearningModule::~LearningModule() {
    LOG_DEFAULT(LogLevel::INFO, "LearningModule: Destructor called.");
}

void LearningModule::learnFromText(const std::string& text,
                                   const std::string& source,
                                   const std::string& topic,
                                   float confidence)
{
    Capsule c;
    c.id = "capsule_" + std::to_string(++s_learning_module_capsule_id_counter); 
    c.topic = topic;
    c.source = source;
    c.content = text; 
    c.confidence = confidence;
    c.plain_text_summary = text.substr(0, std::min((size_t)100, text.length())) + "...";
    c.timestamp_utc = std::chrono::system_clock::now();
    
    c.embedding = this->compute_embedding(c.content); 
    c.cryptofig_blob_base64 = this->cryptofig_encode(c.embedding); 

    std::string my_aes_key = this->get_aes_key_for_peer("Self"); 
    std::string iv = this->generate_random_bytes(EVP_CIPHER_iv_length(EVP_aes_256_gcm())); 
    c.encryption_iv_base64 = this->base64_encode_string(iv); // Yeni public metot kullanıldı
    c.encrypted_content = this->aes_gcm_encrypt(c.content, my_aes_key, iv); 

    // Ed25519 fonksiyonları yorum satırı yapıldığı için burada da simüle ediyoruz
    // std::string my_private_key = this->get_my_private_key(); 
    // c.signature_base64 = this->ed25519_sign(c.encrypted_content, my_private_key); 
    c.signature_base64 = "valid_signature_placeholder"; // Geçici simülasyon

    knowledgeBase.add_capsule(c); 
    knowledgeBase.save(); 
    LOG_DEFAULT(LogLevel::INFO, "LearningModule: Learned from text. Topic: " << topic << ", ID: " << c.id);
}

void LearningModule::learnFromWeb(const std::string& query) {
    WebFetcher fetcher;
    auto results = fetcher.search(query);

    for (auto& r : results) {
        learnFromText(r.content, r.source, query, 0.9f);
    }
    LOG_DEFAULT(LogLevel::INFO, "LearningModule: Learned from web query: " << query);
}

std::vector<Capsule> LearningModule::search_by_topic(const std::string& topic) const {
    return knowledgeBase.search_by_topic(topic);
}

KnowledgeBase& LearningModule::getKnowledgeBase() {
    return knowledgeBase;
}

const KnowledgeBase& LearningModule::getKnowledgeBase() const {
    return knowledgeBase;
}


void LearningModule::process_ai_insights(const std::vector<AIInsight>& insights) {
    LOG_DEFAULT(LogLevel::INFO, "[LearningModule] AI Insights isleniyor: " << insights.size() << " adet içgörü.");
    for (const auto& insight : insights) {
        Capsule c;
        c.id = "insight_" + std::to_string(++s_learning_module_capsule_id_counter); 
        c.content = insight.observation; 
        c.source = "AIInsightsEngine";
        c.topic = "AI Insight"; 
        c.confidence = insight.urgency; 
        c.plain_text_summary = insight.observation.substr(0, std::min((size_t)100, insight.observation.length())) + "...";
        c.timestamp_utc = std::chrono::system_clock::now();
        
        c.embedding = this->compute_embedding(c.content); 
        c.cryptofig_blob_base64 = this->cryptofig_encode(c.embedding); 

        std::string my_aes_key = this->get_aes_key_for_peer("Self"); 
        std::string iv = this->generate_random_bytes(EVP_CIPHER_iv_length(EVP_aes_256_gcm())); 
        c.encryption_iv_base64 = this->base64_encode_string(iv); // Yeni public metot kullanıldı
        c.encrypted_content = this->aes_gcm_encrypt(c.content, my_aes_key, iv); 

        // Ed25519 fonksiyonları yorum satırı yapıldığı için burada da simüle ediyoruz
        // std::string my_private_key = this->get_my_private_key();
        // c.signature_base64 = this->ed25519_sign(c.encrypted_content, my_private_key);
        c.signature_base64 = "valid_signature_placeholder"; // Geçici simülasyon

        knowledgeBase.add_capsule(c); 
        LOG_DEFAULT(LogLevel::INFO, "[LearningModule] KnowledgeBase'e içgörü kapsülü eklendi: " << c.content.substr(0, std::min((size_t)30, c.content.length())) << "..., ID: " << c.id);
    }
    knowledgeBase.save(); 
}

IngestReport LearningModule::ingest_envelope(const Capsule& envelope, const std::string& signature, const std::string& sender_id) {
    IngestReport report;
    report.original_capsule = envelope;
    report.timestamp = std::chrono::system_clock::now();
    report.source_peer_id = sender_id;
    report.processed_capsule = envelope; 

    LOG_DEFAULT(LogLevel::INFO, "LearningModule: Ingesting envelope from " << sender_id << " with ID: " << envelope.id << "...");

    if (!this->verify_signature(report.processed_capsule, signature, sender_id)) { 
        report.result = IngestResult::InvalidSignature;
        report.message = "Signature verification failed.";
        this->audit_log_append(report); 
        knowledgeBase.quarantine_capsule(report.processed_capsule.id); 
        return report;
    }
    LOG_DEFAULT(LogLevel::DEBUG, "LearningModule: Signature verified for capsule ID: " << envelope.id);

    Capsule decrypted_capsule = this->decrypt_payload(report.processed_capsule); 
    if (decrypted_capsule.content.empty() && !report.processed_capsule.encrypted_content.empty()) { 
        report.result = IngestResult::DecryptionFailed;
        report.message = "Payload decryption failed.";
        this->audit_log_append(report); 
        knowledgeBase.quarantine_capsule(report.processed_capsule.id); 
        return report;
    }
    report.processed_capsule = decrypted_capsule; 
    LOG_DEFAULT(LogLevel::DEBUG, "LearningModule: Payload decrypted for capsule ID: " << envelope.id);

    if (!this->schema_validate(report.processed_capsule)) { 
        report.result = IngestResult::SchemaMismatch;
        report.message = "Capsule schema mismatch.";
        this->audit_log_append(report); 
        knowledgeBase.quarantine_capsule(report.processed_capsule.id); 
        return report;
    }
    LOG_DEFAULT(LogLevel::DEBUG, "LearningModule: Schema validated for capsule ID: " << envelope.id);

    Capsule sanitized_capsule = this->sanitize_unicode(report.processed_capsule); 
    if (sanitized_capsule.content != report.processed_capsule.content) { 
        report.result = IngestResult::SanitizationNeeded; 
        report.message = "Unicode sanitization applied.";
        report.processed_capsule = sanitized_capsule; 
        LOG_DEFAULT(LogLevel::INFO, "LearningModule: Unicode sanitization applied to capsule ID: " << envelope.id);
    } else {
        LOG_DEFAULT(LogLevel::DEBUG, "LearningModule: Unicode sanitation not needed for capsule ID: " << envelope.id);
    }
    
    if (this->run_steganalysis(report.processed_capsule)) { 
        report.result = IngestResult::SteganographyDetected;
        report.message = "Steganography detected in capsule content.";
        this->audit_log_append(report); 
        knowledgeBase.quarantine_capsule(report.processed_capsule.id); 
        return report;
    }
    LOG_DEFAULT(LogLevel::DEBUG, "LearningModule: Steganography check passed for capsule ID: " << envelope.id);

    if (!this->sandbox_analysis(report.processed_capsule)) { 
        report.result = IngestResult::SandboxFailed;
        report.message = "Sandbox analysis indicated potential threat.";
        this->audit_log_append(report); 
        knowledgeBase.quarantine_capsule(report.processed_capsule.id); 
        return report;
    }
    LOG_DEFAULT(LogLevel::DEBUG, "LearningModule: Sandbox analysis passed for capsule ID: " << envelope.id);

    if (!this->corroboration_check(report.processed_capsule)) { 
        report.result = IngestResult::CorroborationFailed;
        report.message = "Corroboration check failed against existing knowledge.";
        this->audit_log_append(report); 
        knowledgeBase.quarantine_capsule(report.processed_capsule.id); 
        return report;
    }
    LOG_DEFAULT(LogLevel::DEBUG, "LearningModule: Corroboration check passed for capsule ID: " << envelope.id);

    knowledgeBase.add_capsule(report.processed_capsule); 
    knowledgeBase.save(); 
    report.result = IngestResult::Success;
    report.message = "Capsule ingested successfully.";
    this->audit_log_append(report); 
    LOG_DEFAULT(LogLevel::INFO, "LearningModule: Capsule ingested successfully from " << sender_id << ", ID: " << envelope.id);
    return report;
}

bool LearningModule::verify_signature(const Capsule& capsule, const std::string& signature, const std::string& sender_id) const {
    if (sender_id == "Self") { 
        return true;
    }

    if (capsule.signature_base64.empty()) { 
        LOG_DEFAULT(LogLevel::WARNING, "LearningModule: Signature missing in capsule ID: " << capsule.id << " from " << sender_id);
        return false;
    }

    // Ed25519 doğrulamasını şimdilik simüle ediyoruz
    // std::string message_to_verify = capsule.encrypted_content;
    // std::string signature_bytes = this->base64_decode_string(capsule.signature_base64); // Yeni public metot
    // std::string public_key_bytes = this->get_public_key_for_peer(sender_id); 
    // return this->ed25519_verify(message_to_verify, signature_bytes, public_key_bytes); 
    
    // Geçici olarak sadece gelen signature ile "valid_signature" stringini karşılaştırıyoruz.
    return signature == "valid_signature"; 
}

Capsule LearningModule::decrypt_payload(const Capsule& encrypted_capsule) const {
    Capsule decrypted = encrypted_capsule;

    if (encrypted_capsule.encrypted_content.empty()) {
        LOG_DEFAULT(LogLevel::DEBUG, "LearningModule: Encrypted content is empty for capsule ID: " << encrypted_capsule.id << ". Assuming plain text.");
        decrypted.content = encrypted_capsule.content; 
        return decrypted;
    }

    std::string aes_key = this->get_aes_key_for_peer(encrypted_capsule.source); 
    std::string iv = this->base64_decode_string(encrypted_capsule.encryption_iv_base64); // Yeni public metot

    decrypted.content = this->aes_gcm_decrypt(encrypted_capsule.encrypted_content, aes_key, iv); 

    if (decrypted.content.empty() && !encrypted_capsule.encrypted_content.empty()) {
        LOG_DEFAULT(LogLevel::WARNING, "LearningModule: Decryption resulted in empty content for capsule ID: " << encrypted_capsule.id);
    }
    return decrypted;
}

bool LearningModule::schema_validate(const Capsule& capsule) const { 
    bool valid = !capsule.id.empty() && !capsule.content.empty() && !capsule.source.empty() && !capsule.topic.empty() && 
                 !capsule.cryptofig_blob_base64.empty() && !capsule.signature_base64.empty() && !capsule.encryption_iv_base64.empty();

    if (!valid) {
        LOG_DEFAULT(LogLevel::WARNING, "LearningModule: Schema validation failed for capsule ID: " << capsule.id << ". Missing required fields.");
    }
    return valid;
}

Capsule LearningModule::sanitize_unicode(const Capsule& capsule) const { 
    Capsule sanitized_capsule = capsule;
    if (this->unicodeSanitizer) { 
        sanitized_capsule.content = this->unicodeSanitizer->sanitize(capsule.content); 
        if (sanitized_capsule.content != capsule.content) {
            LOG_DEFAULT(LogLevel::INFO, "LearningModule: Unicode sanitization changed content for capsule ID: " << capsule.id);
        }
    } else {
        LOG_DEFAULT(LogLevel::WARNING, "LearningModule: UnicodeSanitizer not available for sanitization for capsule ID: " << capsule.id);
    }
    return sanitized_capsule;
}

bool LearningModule::run_steganalysis(const Capsule& capsule) const { 
    if (this->stegoDetector) { 
        if (this->stegoDetector->detectSteganography(capsule.content)) { 
            LOG_DEFAULT(LogLevel::WARNING, "LearningModule: Steganography detected in capsule ID: " << capsule.id);
            return true;
        }
    } else {
        LOG_DEFAULT(LogLevel::WARNING, "LearningModule: StegoDetector not available for steganalysis for capsule ID: " << capsule.id);
    }
    return false;
}

bool LearningModule::sandbox_analysis(const Capsule& capsule) const { 
    if (capsule.content.find("malware_signature") != std::string::npos ||
        capsule.content.find("exploit_code") != std::string::npos) {
        LOG_DEFAULT(LogLevel::ERR_CRITICAL, "LearningModule: Sandbox analysis detected potential threat in capsule ID: " << capsule.id);
        return false; 
    }
    return true; 
}

bool LearningModule::corroboration_check(const Capsule& capsule) const { 
    auto similar_capsules = knowledgeBase.semantic_search(capsule.content, 1); 
    if (!similar_capsules.empty() && similar_capsules[0].confidence > 0.95f && similar_capsules[0].id != capsule.id) { 
        LOG_DEFAULT(LogLevel::WARNING, "LearningModule: Corroboration check found highly similar existing knowledge for capsule ID: " << capsule.id << ". Similar to existing capsule ID: " << similar_capsules[0].id);
    }
    return true; 
}

void LearningModule::audit_log_append(const IngestReport& report) const { 
    LOG_DEFAULT(LogLevel::WARNING, "LearningModule: Ingest Audit Report - Result: " << static_cast<int>(report.result) 
                                      << ", Message: " << report.message 
                                      << ", Source Peer: " << report.source_peer_id 
                                      << ", Original Capsule ID: " << report.original_capsule.id);
}

// Kriptografik ve Embedding Altyapısı Implementasyonları (İskeletler)

std::vector<float> LearningModule::compute_embedding(const std::string& text) const {
    LOG_DEFAULT(LogLevel::DEBUG, "LearningModule: compute_embedding (KnowledgeBase'den) çağrıldı.");
    return knowledgeBase.computeEmbedding(text); 
}

std::string LearningModule::cryptofig_encode(const std::vector<float>& cryptofig_vector) const {
    nlohmann::json j = cryptofig_vector;
    std::string serialized_cryptofig = j.dump();
    return this->base64_encode_internal(serialized_cryptofig); // this-> eklendi
}

std::vector<float> LearningModule::cryptofig_decode_base64(const std::string& base64_cryptofig_blob) const {
    std::string serialized_cryptofig = this->base64_decode_internal(base64_cryptofig_blob); // this-> eklendi
    try {
        nlohmann::json j = nlohmann::json::parse(serialized_cryptofig);
        return j.get<std::vector<float>>();
    } catch (const nlohmann::json::parse_error& e) {
        LOG_DEFAULT(LogLevel::ERR_CRITICAL, "LearningModule: cryptofig_decode_base64 JSON ayrıştırma hatası: " << e.what());
        return {};
    }
}

std::string LearningModule::aes_gcm_encrypt(const std::string& plaintext, const std::string& key, const std::string& iv) const {
    EVP_CIPHER_CTX *ctx;
    int len;
    int ciphertext_len;

    int max_len = plaintext.length() + EVP_CIPHER_block_size(EVP_aes_256_gcm()) + 16;
    unsigned char* ciphertext_buf = (unsigned char*)OPENSSL_malloc(max_len);
    unsigned char* tag_buf = (unsigned char*)OPENSSL_malloc(16); 

    if (!ciphertext_buf || !tag_buf) {
        LOG_DEFAULT(LogLevel::ERR_CRITICAL, "AES-GCM encrypt için bellek ayrılamadı.");
        OPENSSL_free(ciphertext_buf);
        OPENSSL_free(tag_buf);
        return "";
    }

    if (!(ctx = EVP_CIPHER_CTX_new())) {
        ERR_print_errors_fp(stderr);
        OPENSSL_free(ciphertext_buf);
        OPENSSL_free(tag_buf);
        return "";
    }

    if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL)) {
        ERR_print_errors_fp(stderr);
        EVP_CIPHER_CTX_free(ctx);
        OPENSSL_free(ciphertext_buf);
        OPENSSL_free(tag_buf);
        return "";
    }

    if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, (int)iv.length(), NULL)) {
        ERR_print_errors_fp(stderr);
        EVP_CIPHER_CTX_free(ctx);
        OPENSSL_free(ciphertext_buf);
        OPENSSL_free(tag_buf);
        return "";
    }

    if (1 != EVP_EncryptInit_ex(ctx, NULL, NULL, (const unsigned char*)key.c_str(), (const unsigned char*)iv.c_str())) {
        ERR_print_errors_fp(stderr);
        EVP_CIPHER_CTX_free(ctx);
        OPENSSL_free(ciphertext_buf);
        OPENSSL_free(tag_buf); 
        return "";
    }

    if (1 != EVP_EncryptUpdate(ctx, ciphertext_buf, &len, (const unsigned char*)plaintext.c_str(), static_cast<int>(plaintext.length()))) {
        ERR_print_errors_fp(stderr);
        EVP_CIPHER_CTX_free(ctx);
        OPENSSL_free(ciphertext_buf);
        OPENSSL_free(tag_buf);
        return "";
    }
    ciphertext_len = len;

    if (1 != EVP_EncryptFinal_ex(ctx, ciphertext_buf + len, &len)) {
        ERR_print_errors_fp(stderr);
        EVP_CIPHER_CTX_free(ctx);
        OPENSSL_free(ciphertext_buf);
        OPENSSL_free(tag_buf);
        return "";
    }
    ciphertext_len += len;

    if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag_buf)) {
        ERR_print_errors_fp(stderr);
        EVP_CIPHER_CTX_free(ctx);
        OPENSSL_free(ciphertext_buf);
        OPENSSL_free(tag_buf);
        return "";
    }

    EVP_CIPHER_CTX_free(ctx);

    std::string result((char*)ciphertext_buf, ciphertext_len);
    result.append((char*)tag_buf, 16); 

    OPENSSL_free(ciphertext_buf);
    OPENSSL_free(tag_buf);
    return this->base64_encode_internal(result); // this-> eklendi
}

std::string LearningModule::aes_gcm_decrypt(const std::string& ciphertext_base64, const std::string& key, const std::string& iv) const {
    std::string combined_data = this->base64_decode_internal(ciphertext_base64); // this-> eklendi

    size_t tag_len = 16; 
    if (combined_data.length() < tag_len) {
        LOG_DEFAULT(LogLevel::ERR_CRITICAL, "AES-GCM decrypt: Geçersiz ciphertext uzunluğu (tag eksik).");
        return "";
    }

    std::string ciphertext_str = combined_data.substr(0, combined_data.length() - tag_len);
    std::string tag_str = combined_data.substr(combined_data.length() - tag_len);

    EVP_CIPHER_CTX *ctx;
    int len;
    int plaintext_len;

    unsigned char* plaintext_buf = (unsigned char*)OPENSSL_malloc(ciphertext_str.length() + EVP_CIPHER_block_size(EVP_aes_256_gcm()));
    if (!plaintext_buf) {
        LOG_DEFAULT(LogLevel::ERR_CRITICAL, "AES-GCM decrypt için bellek ayrılamadı.");
        return "";
    }

    if (!(ctx = EVP_CIPHER_CTX_new())) {
        ERR_print_errors_fp(stderr);
        OPENSSL_free(plaintext_buf);
        return "";
    }

    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL)) {
        ERR_print_errors_fp(stderr);
        EVP_CIPHER_CTX_free(ctx);
        OPENSSL_free(plaintext_buf);
        return "";
    }

    if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, (int)iv.length(), NULL)) {
        ERR_print_errors_fp(stderr);
        EVP_CIPHER_CTX_free(ctx);
        OPENSSL_free(plaintext_buf);
        return "";
    }

    if (1 != EVP_DecryptInit_ex(ctx, NULL, NULL, (const unsigned char*)key.c_str(), (const unsigned char*)iv.c_str())) {
        ERR_print_errors_fp(stderr);
        EVP_CIPHER_CTX_free(ctx);
        OPENSSL_free(plaintext_buf);
        return "";
    }

    if (1 != EVP_DecryptUpdate(ctx, plaintext_buf, &len, (const unsigned char*)ciphertext_str.c_str(), static_cast<int>(ciphertext_str.length()))) {
        ERR_print_errors_fp(stderr);
        EVP_CIPHER_CTX_free(ctx);
        OPENSSL_free(plaintext_buf);
        return "";
    }
    plaintext_len = len;

    if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, (int)tag_str.length(), (void*)tag_str.c_str())) {
        ERR_print_errors_fp(stderr);
        EVP_CIPHER_CTX_free(ctx);
        OPENSSL_free(plaintext_buf);
        return "";
    }

    if (1 != EVP_DecryptFinal_ex(ctx, plaintext_buf + len, &len)) {
        ERR_print_errors_fp(stderr); 
        EVP_CIPHER_CTX_free(ctx);
        OPENSSL_free(plaintext_buf);
        return "";
    }
    plaintext_len += len;

    EVP_CIPHER_CTX_free(ctx);

    std::string result((char*)plaintext_buf, plaintext_len);
    OPENSSL_free(plaintext_buf);
    return result;
}

// Ed25519 ile ilgili fonksiyonlar şimdilik yorum satırı
// std::string LearningModule::ed25519_sign(const std::string& message, const std::string& private_key) const { /* ... */ }
// bool LearningModule::ed25519_verify(const std::string& message, const std::string& signature_base64, const std::string& public_key) const { /* ... */ }

std::string LearningModule::generate_random_bytes(size_t length) const {
    unsigned char* buffer = (unsigned char*)OPENSSL_malloc(length);
    if (!buffer) {
        LOG_DEFAULT(LogLevel::ERR_CRITICAL, "RAND_bytes için bellek ayrılamadı.");
        return "";
    }
    if (1 != RAND_bytes(buffer, static_cast<int>(length))) {
        ERR_print_errors_fp(stderr);
        OPENSSL_free(buffer);
        return "";
    }
    std::string random_data((char*)buffer, length);
    OPENSSL_free(buffer);
    return random_data;
}

std::string LearningModule::get_aes_key_for_peer(const std::string& peer_id) const {
    if (peer_id == "Self") {
        return std::string(32, 'S'); 
    }
    return std::string(32, 'A'); 
}

std::string LearningModule::get_public_key_for_peer(const std::string& peer_id) const {
    // Normalde Ed25519 public key uzunluğu 32'dir
    if (peer_id == "Self") {
        return std::string(DUMMY_ED25519_PUBKEY_LEN, 'P'); 
    } else if (peer_id == "Test_Peer_A") {
        return std::string(DUMMY_ED25519_PUBKEY_LEN, 'A'); 
    } else if (peer_id == "Unauthorized_Peer") {
        return std::string(DUMMY_ED25519_PUBKEY_LEN, 'U'); 
    } else if (peer_id == "Suspicious_Source") {
        return std::string(DUMMY_ED25519_PUBKEY_LEN, 'X'); 
    } else if (peer_id == "Dirty_Source") {
        return std::string(DUMMY_ED25519_PUBKEY_LEN, 'D'); 
    }
    return std::string(DUMMY_ED25519_PUBKEY_LEN, 'K'); 
}

std::string LearningModule::get_my_private_key() const {
    // Normalde Ed25519 private key uzunluğu 64'tür (32 byte seed + 32 byte public key)
    return std::string(DUMMY_ED25519_PRIVKEY_LEN, 'M'); 
}

std::string LearningModule::get_my_public_key() const {
    return std::string(DUMMY_ED25519_PUBKEY_LEN, 'P'); 
}