#ifndef LEARNINGMODULE_H
#define LEARNINGMODULE_H

#include <string>
#include <vector> 
#include <map> 
#include <chrono> 
#include <memory> // std::unique_ptr için

#include "KnowledgeBase.h"
#include "../communication/ai_insights_engine.h" 

// YENİ: Kapsül işleme sonucunu belirten enum
enum class IngestResult {
    Success,
    InvalidSignature,
    DecryptionFailed,
    SchemaMismatch,
    SanitizationNeeded,
    SteganographyDetected,
    SandboxFailed,
    CorroborationFailed,
    UnknownError,
    Ignored // Örneğin, zaten mevcut bir kapsül için
};

// YENİ: Kapsül işleme raporu
struct IngestReport {
    Capsule original_capsule; // Orijinal kapsül
    Capsule processed_capsule; // İşlenmiş/değiştirilmiş kapsül
    IngestResult result;      // İşlem sonucu
    std::string message;      // Detaylı mesaj
    std::string source_peer_id; // Kapsülün geldiği peer ID'si
    std::chrono::system_clock::time_point timestamp; // İşlem zamanı
    std::map<std::string, std::string> diagnostics; // Ek teşhis bilgileri
};

// YENİ: İleri bildirimler (aşağıda sınıf tanımlarını yapacağımız için)
class UnicodeSanitizer;
class StegoDetector;

// Geçici olarak Ed25519 sabitleri tanımlanıyor, OpenSSL bulunana kadar
// EVP_PKEY_ED25519 ve ilgili boyutlar EVP API'leri üzerinden yönetilecek.
#define DUMMY_ED25519_PRIVKEY_LEN 64 
#define DUMMY_ED25519_PUBKEY_LEN 32  
#define DUMMY_ED25519_SIG_LEN   64   

class LearningModule {
public:
    LearningModule(KnowledgeBase& kb);
    ~LearningModule(); 

    void learnFromText(const std::string& text,
                       const std::string& source,
                       const std::string& topic,
                       float confidence = 1.0f);

    void learnFromWeb(const std::string& query); 

    virtual std::vector<Capsule> search_by_topic(const std::string& topic) const; 

    void process_ai_insights(const std::vector<AIInsight>& insights);

    virtual KnowledgeBase& getKnowledgeBase(); 
    virtual const KnowledgeBase& getKnowledgeBase() const; 

    IngestReport ingest_envelope(const Capsule& envelope, const std::string& signature, const std::string& sender_id);

    virtual std::vector<float> compute_embedding(const std::string& text) const; 
    virtual std::string cryptofig_encode(const std::vector<float>& cryptofig_vector) const; 
    virtual std::vector<float> cryptofig_decode_base64(const std::string& base64_cryptofig_blob) const; 
    virtual std::string aes_gcm_encrypt(const std::string& plaintext, const std::string& key, const std::string& iv) const; 
    virtual std::string aes_gcm_decrypt(const std::string& ciphertext, const std::string& key, const std::string& iv) const; 
    
    virtual std::string ed25519_sign(const std::string& message, const std::string& private_key_pem) const; 
    virtual bool ed25519_verify(const std::string& message, const std::string& signature_base64, const std::string& public_key_pem) const; 
    virtual std::string generate_random_bytes(size_t length) const; 

    virtual std::string get_aes_key_for_peer(const std::string& peer_id) const;
    virtual std::string get_public_key_for_peer(const std::string& peer_id) const; 
    virtual std::string get_my_private_key() const;                               
    virtual std::string get_my_public_key() const;                                

    // YENİ: String için public Base64 encode/decode metotları (Bunlar LearningModule.cpp'de implemente edilecek)
    virtual std::string base64_encode_string(const std::string& data) const; 
    virtual std::string base64_decode_string(const std::string& data) const; 

private:
    KnowledgeBase& knowledgeBase; // Düzeltme: Üye değişkeni tanımı burada olmalı
    std::unique_ptr<UnicodeSanitizer> unicodeSanitizer; // Düzeltme: Üye değişkeni tanımı burada olmalı
    std::unique_ptr<StegoDetector> stegoDetector;       // Düzeltme: Üye değişkeni tanımı burada olmalı

    // ingest_envelope pipeline'ı içindeki özel metodlar
    bool verify_signature(const Capsule& capsule, const std::string& signature, const std::string& sender_id) const;
    Capsule decrypt_payload(const Capsule& encrypted_capsule) const;
    bool schema_validate(const Capsule& capsule) const;
    Capsule sanitize_unicode(const Capsule& capsule) const;
    bool run_steganalysis(const Capsule& capsule) const;
    bool sandbox_analysis(const Capsule& capsule) const;
    bool corroboration_check(const Capsule& capsule) const;
    void audit_log_append(const IngestReport& report) const; 

    // Base64 kodlama/kod çözme fonksiyonları LearningModule'ün private üyeleri
    std::string base64_encode_internal(const std::string& in) const; 
    std::string base64_decode_internal(const std::string& in) const; 
};

#endif // LEARNINGMODULE_H