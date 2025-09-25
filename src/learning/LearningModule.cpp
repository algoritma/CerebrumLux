#include "LearningModule.h"
#include <iostream>
#include <algorithm>
#include "../core/logger.h"
#include "../learning/WebFetcher.h"
#include "../core/utils.h"
#include "../external/nlohmann/json.hpp"
#include "../crypto/CryptoUtils.h" // CryptoUtils'ın Base64 fonksiyonları için

// YENİ: UnicodeSanitizer ve StegoDetector başlık dosyaları
#include "UnicodeSanitizer.h" // Tam tanımlar için eklendi
#include "StegoDetector.h"    // Tam tanımlar için eklendi

// Kapsül ID sayacını string ID'lere uyarlamak için (isteğe bağlı)
static unsigned int s_learning_module_capsule_id_counter = 0;

namespace CerebrumLux { // Buradan itibaren CerebrumLux namespace'i başlar

// Kurucu
LearningModule::LearningModule(KnowledgeBase& kb, CerebrumLux::Crypto::CryptoManager& cryptoMan)
    : knowledgeBase(kb),
      cryptoManager(cryptoMan), // Yeni: CryptoManager referansını başlat
      unicodeSanitizer(std::make_unique<UnicodeSanitizer>()), // Tam tanım artık mevcut
      stegoDetector(std::make_unique<StegoDetector>())      // Tam tanım artık mevcut
{
    LOG_DEFAULT(LogLevel::INFO, "LearningModule: Initialized with CryptoManager.");
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

    std::vector<unsigned char> my_aes_key_vec = cryptoManager.generate_random_bytes_vec(32); // 256-bit AES key
    std::vector<unsigned char> iv_vec = cryptoManager.generate_random_bytes_vec(12); // 12 byte GCM IV
    c.encryption_iv_base64 = CerebrumLux::Crypto::base64_encode(cryptoManager.vec_to_str(iv_vec));

    CerebrumLux::Crypto::AESGCMCiphertext encrypted_data =
        cryptoManager.aes256_gcm_encrypt(cryptoManager.str_to_vec(c.content), my_aes_key_vec, {}); // AAD boş bırakıldı

    c.encrypted_content = encrypted_data.ciphertext_base64;
    c.gcm_tag_base64 = encrypted_data.tag_base64; // GCM tag'ını yeni alana atıyoruz

    std::string my_private_key_pem = cryptoManager.get_my_private_key_pem();
    c.signature_base64 = cryptoManager.ed25519_sign(c.encrypted_content, my_private_key_pem);

    this->knowledgeBase.add_capsule(c);
    this->knowledgeBase.save();
    LOG_DEFAULT(LogLevel::INFO, "LearningModule: Learned from text. Topic: " << topic << ", ID: " << c.id);
}

void LearningModule::learnFromWeb(const std::string& query) {
    WebFetcher fetcher; // WebFetcher'ın da CerebrumLux namespace'i içinde olduğunu varsayıyoruz
    auto results = fetcher.search(query);

    for (auto& r : results) {
        learnFromText(r.content, r.source, query, 0.9f);
    }
    LOG_DEFAULT(LogLevel::INFO, "LearningModule: Learned from web query: " << query);
}

std::vector<Capsule> LearningModule::search_by_topic(const std::string& topic) const {
    return this->knowledgeBase.search_by_topic(topic);
}

KnowledgeBase& LearningModule::getKnowledgeBase() {
    return this->knowledgeBase;
}

const KnowledgeBase& LearningModule::getKnowledgeBase() const {
    return this->knowledgeBase;
}


void LearningModule::process_ai_insights(const std::vector<AIInsight>& insights) {
    LOG_DEFAULT(LogLevel::INFO, "[LearningModule] AI Insights isleniyor: " << insights.size() << " adet içgörü.");
    for (const auto& insight : insights) {
        Capsule c;
        c.id = "insight_" + std::to_string(++s_learning_module_capsule_id_counter);
        c.content = insight.observation;
        c.source = "AIInsightsEngine"; // AIInsightsEngine'ın da CerebrumLux namespace'i içinde olduğunu varsayıyoruz
        c.topic = "AI Insight";
        c.confidence = static_cast<float>(insight.urgency); // UrgencyLevel'dan float'a açık dönüşüm
        c.plain_text_summary = insight.observation.substr(0, std::min((size_t)100, insight.observation.length())) + "...";
        c.timestamp_utc = std::chrono::system_clock::now();

        c.embedding = this->compute_embedding(c.content);
        c.cryptofig_blob_base64 = this->cryptofig_encode(c.embedding);

        std::vector<unsigned char> my_aes_key_vec = cryptoManager.generate_random_bytes_vec(32);
        std::vector<unsigned char> iv_vec = cryptoManager.generate_random_bytes_vec(12);
        c.encryption_iv_base64 = CerebrumLux::Crypto::base64_encode(cryptoManager.vec_to_str(iv_vec));

        CerebrumLux::Crypto::AESGCMCiphertext encrypted_data =
            cryptoManager.aes256_gcm_encrypt(cryptoManager.str_to_vec(c.content), my_aes_key_vec, {});

        c.encrypted_content = encrypted_data.ciphertext_base64;
        c.gcm_tag_base64 = encrypted_data.tag_base64;

        std::string my_private_key_pem = cryptoManager.get_my_private_key_pem();
        c.signature_base64 = cryptoManager.ed25519_sign(c.encrypted_content, my_private_key_pem);

        this->knowledgeBase.add_capsule(c);
        LOG_DEFAULT(LogLevel::INFO, "[LearningModule] KnowledgeBase'e içgörü kapsülü eklendi: " << c.content.substr(0, std::min((size_t)30, c.content.length())) << "..., ID: " << c.id);
    }
    this->knowledgeBase.save();
}

IngestReport LearningModule::ingest_envelope(const Capsule& envelope, const std::string& signature, const std::string& sender_id) {
    IngestReport report;
    report.original_capsule = envelope;
    report.timestamp = std::chrono::system_clock::now();
    report.source_peer_id = sender_id;
    report.processed_capsule = envelope;

    LOG_DEFAULT(LogLevel::INFO, "LearningModule: Ingesting envelope from " << sender_id << " with ID: " << envelope.id << "...");

    std::string public_key_of_sender_pem = cryptoManager.get_peer_public_key_pem(sender_id);
    if (!cryptoManager.ed25519_verify(report.processed_capsule.encrypted_content, report.processed_capsule.signature_base64, public_key_of_sender_pem)) {
        report.result = IngestResult::InvalidSignature;
        report.message = "Signature verification failed.";
        this->audit_log_append(report);
        this->knowledgeBase.quarantine_capsule(report.processed_capsule.id);
        return report;
    }
    LOG_DEFAULT(LogLevel::DEBUG, "LearningModule: Signature verified for capsule ID: " << envelope.id);

    Capsule decrypted_capsule = this->decrypt_payload(report.processed_capsule);
    if (decrypted_capsule.content.empty() && !report.processed_capsule.encrypted_content.empty()) {
        report.result = IngestResult::DecryptionFailed;
        report.message = "Payload decryption failed.";
        this->audit_log_append(report);
        this->knowledgeBase.quarantine_capsule(report.processed_capsule.id);
        return report;
    }
    report.processed_capsule = decrypted_capsule;
    LOG_DEFAULT(LogLevel::DEBUG, "LearningModule: Payload decrypted for capsule ID: " << envelope.id);

    if (!this->schema_validate(report.processed_capsule)) {
        report.result = IngestResult::SchemaMismatch;
        report.message = "Capsule schema mismatch.";
        this->audit_log_append(report);
        this->knowledgeBase.quarantine_capsule(report.processed_capsule.id);
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
        this->knowledgeBase.quarantine_capsule(report.processed_capsule.id);
        return report;
    }
    LOG_DEFAULT(LogLevel::DEBUG, "LearningModule: Steganography check passed for capsule ID: " << envelope.id);

    if (!this->sandbox_analysis(report.processed_capsule)) {
        report.result = IngestResult::SandboxFailed;
        report.message = "Sandbox analysis indicated potential threat.";
        this->audit_log_append(report);
        this->knowledgeBase.quarantine_capsule(report.processed_capsule.id);
        return report;
    }
    LOG_DEFAULT(LogLevel::DEBUG, "LearningModule: Sandbox analysis passed for capsule ID: " << envelope.id);

    if (!this->corroboration_check(report.processed_capsule)) {
        report.result = IngestResult::CorroborationFailed;
        report.message = "Corroboration check failed against existing knowledge.";
        this->audit_log_append(report);
        this->knowledgeBase.quarantine_capsule(report.processed_capsule.id);
        return report;
    }
    LOG_DEFAULT(LogLevel::DEBUG, "LearningModule: Corroboration check passed for capsule ID: " << envelope.id);

    this->knowledgeBase.add_capsule(report.processed_capsule);
    this->knowledgeBase.save();
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

    std::string message_to_verify = capsule.encrypted_content;
    std::string public_key_bytes_pem = cryptoManager.get_peer_public_key_pem(sender_id);
    return cryptoManager.ed25519_verify(message_to_verify, capsule.signature_base64, public_key_bytes_pem);
}

Capsule LearningModule::decrypt_payload(const Capsule& encrypted_capsule) const {
    Capsule decrypted = encrypted_capsule;

    if (encrypted_capsule.encrypted_content.empty()) {
        LOG_DEFAULT(LogLevel::DEBUG, "LearningModule: Encrypted content is empty for capsule ID: " << encrypted_capsule.id << ". Assuming plain text.");
        decrypted.content = encrypted_capsule.content;
        return decrypted;
    }

    std::vector<unsigned char> aes_key_for_peer_vec = cryptoManager.generate_random_bytes_vec(32); // Geçici, ECDH ile değişecek

    CerebrumLux::Crypto::AESGCMCiphertext ct_data;
    ct_data.ciphertext_base64 = encrypted_capsule.encrypted_content;
    ct_data.tag_base64 = encrypted_capsule.gcm_tag_base64;
    ct_data.iv_base64 = encrypted_capsule.encryption_iv_base64;

    try {
        std::vector<unsigned char> decrypted_vec = cryptoManager.aes256_gcm_decrypt(
            cryptoManager.str_to_vec(CerebrumLux::Crypto::base64_decode(ct_data.ciphertext_base64)),
            cryptoManager.str_to_vec(CerebrumLux::Crypto::base64_decode(ct_data.tag_base64)),
            cryptoManager.str_to_vec(CerebrumLux::Crypto::base64_decode(ct_data.iv_base64)),
            aes_key_for_peer_vec,
            {}); // AAD boş

        decrypted.content = cryptoManager.vec_to_str(decrypted_vec);
    } catch (const std::exception& e) {
        LOG_DEFAULT(LogLevel::WARNING, "LearningModule: Decryption failed for capsule ID: " << encrypted_capsule.id << ". Error: " << e.what());
        decrypted.content.clear(); // Hata durumunda içeriği temizle
    }

    if (decrypted.content.empty() && !encrypted_capsule.encrypted_content.empty()) {
        LOG_DEFAULT(LogLevel::WARNING, "LearningModule: Decryption resulted in empty content for capsule ID: " << encrypted_capsule.id);
    }
    return decrypted;
}

bool LearningModule::schema_validate(const Capsule& capsule) const {
    bool valid = !capsule.id.empty() && !capsule.content.empty() && !capsule.source.empty() && !capsule.topic.empty() &&
                 !capsule.cryptofig_blob_base64.empty() && !capsule.signature_base64.empty() && !capsule.encryption_iv_base64.empty() && !capsule.gcm_tag_base64.empty();

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

std::vector<float> LearningModule::compute_embedding(const std::string& text) const {
    LOG_DEFAULT(LogLevel::DEBUG, "LearningModule: compute_embedding (KnowledgeBase'den) çağrıldı.");
    return this->knowledgeBase.computeEmbedding(text);
}

std::string LearningModule::cryptofig_encode(const std::vector<float>& cryptofig_vector) const {
    nlohmann::json j = cryptofig_vector;
    std::string serialized_cryptofig = j.dump();
    return CerebrumLux::Crypto::base64_encode(serialized_cryptofig);
}

std::vector<float> LearningModule::cryptofig_decode_base64(const std::string& base64_cryptofig_blob) const {
    std::string serialized_cryptofig = CerebrumLux::Crypto::base64_decode(base64_cryptofig_blob);
    try {
        nlohmann::json j = nlohmann::json::parse(serialized_cryptofig);
        return j.get<std::vector<float>>();
    } catch (const nlohmann::json::parse_error& e) {
        LOG_DEFAULT(LogLevel::ERR_CRITICAL, "LearningModule: cryptofig_decode_base64 JSON ayrıştırma hatası: " << e.what());
        return {};
    }
}

} // namespace CerebrumLux