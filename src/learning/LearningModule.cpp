#include "LearningModule.h"
#include "../core/logger.h"
#include "../core/enums.h"
#include "../core/utils.h" // SafeRNG için
#include "../crypto/CryptoManager.h" // cryptoManager için
#include "../crypto/CryptoUtils.h" // Base64 kodlama için
#include <iostream>
#include <algorithm> // std::min için
#include <stdexcept> // std::runtime_error için

namespace CerebrumLux {

LearningModule::LearningModule(KnowledgeBase& kb, CerebrumLux::Crypto::CryptoManager& cryptoMan)
    : knowledgeBase(kb), cryptoManager(cryptoMan),
      unicodeSanitizer(std::make_unique<UnicodeSanitizer>()),
      stegoDetector(std::make_unique<StegoDetector>())
{
    LOG_DEFAULT(LogLevel::INFO, "LearningModule: Initialized with CryptoManager.");
}

LearningModule::~LearningModule() {
    LOG_DEFAULT(LogLevel::INFO, "LearningModule: Destructor called.");
}

void LearningModule::learnFromText(const std::string& text,
                                   const std::string& source,
                                   const std::string& topic,
                                   float confidence) {
    LOG_DEFAULT(LogLevel::INFO, "LearningModule: Metinden öğrenme başlatıldı. Konu: " << topic << ", Kaynak: " << source);

    Capsule new_capsule;
    new_capsule.id = "text_" + std::to_string(get_current_timestamp_us());
    new_capsule.content = text;
    new_capsule.source = source;
    new_capsule.topic = topic;
    new_capsule.confidence = confidence;
    new_capsule.plain_text_summary = text.substr(0, std::min((size_t)100, text.length())) + "...";
    new_capsule.timestamp_utc = std::chrono::system_clock::now();

    // Embedding ve Cryptofig işlemleri
    new_capsule.embedding = compute_embedding(new_capsule.content);
    new_capsule.cryptofig_blob_base64 = cryptofig_encode(new_capsule.embedding);

    // Kapsülü bilgi tabanına ekle
    knowledgeBase.add_capsule(new_capsule);
    LOG_DEFAULT(LogLevel::INFO, "LearningModule: Yeni kapsül bilgi tabanına eklendi. ID: " << new_capsule.id);
}

void LearningModule::learnFromWeb(const std::string& query) {
    LOG_DEFAULT(LogLevel::INFO, "LearningModule: Web'den öğrenme başlatıldı. Sorgu: " << query);
    // Gerçek WebFetcher entegrasyonu burada olacak.
    // Şimdilik sadece bir log mesajı ile placeholder.
    LOG_DEFAULT(LogLevel::WARNING, "LearningModule: learnFromWeb henüz gerçek bir WebFetcher implementasyonuna sahip değil.");

    // Test amaçlı dummy bir kapsül ekleyebiliriz
    // learnFromText("Web araması sonucu: " + query + " için dummy içerik.", "WebFetcher", "WebSearch", 0.7f);
}

std::vector<Capsule> LearningModule::search_by_topic(const std::string& topic) const {
    LOG_DEFAULT(LogLevel::DEBUG, "[LearningModule] Topic'e göre arama yapılıyor: " << topic);
    return knowledgeBase.search_by_topic(topic);
}

void LearningModule::process_ai_insights(const std::vector<AIInsight>& insights) {
    LOG_DEFAULT(LogLevel::INFO, "[LearningModule] AI Insights isleniyor: " << insights.size() << " adet içgörü.");

    for (const auto& insight : insights) {
        Capsule insight_capsule;
        insight_capsule.id = insight.id;
        insight_capsule.content = insight.observation;
        insight_capsule.source = "AIInsightsEngine";
        insight_capsule.topic = "AI Insight"; // Genel bir topic
        insight_capsule.confidence = 0.5f; // İçgörünün kendi güven değeri olabilir, şimdilik sabit
        // Eğer AIInsight objesi bir confidence alanı içeriyorsa, onu kullanabiliriz.
        // Örneğin: insight_capsule.confidence = insight.confidence_score;
        // Ancak current implementation'da AIInsight'ta böyle bir alan yok.

        // Eğer içgörü bir "Sistem Genel Performans Metriği" context'ine sahipse, bunu GraphData topic'i altında kaydet.
        // Bu sayede MainWindow::updateGui() grafiği güncelleyebilir.
        if (insight.context == "Sistem Genel Performans Metriği") { 
            insight_capsule.topic = "GraphData"; // GRAFİK İÇİN TOPIC DÜZELTİLDİ
            // Güven değerini kapsülün confidence'ına yansıtalım.
            // Örneğin: "AI sisteminin anlık güven seviyesi: 0.85" gibi bir metinden 0.85'i çıkarmak.
            size_t pos = insight.observation.find(":");
            if (pos != std::string::npos && pos + 1 < insight.observation.length()) {
                try {
                    insight_capsule.confidence = std::stof(insight.observation.substr(pos + 1));
                    LOG_DEFAULT(LogLevel::DEBUG, "[LearningModule] Grafik verisi için içgörü güveni çıkarıldı: " << insight_capsule.confidence);
                } catch (const std::exception& e) {
                    LOG_DEFAULT(LogLevel::WARNING, "[LearningModule] İçgörüden güven değeri çıkarılırken hata: " << e.what());
                }
            }
        }


        insight_capsule.plain_text_summary = insight.observation.substr(0, std::min((size_t)100, insight.observation.length())) + "...";
        insight_capsule.timestamp_utc = std::chrono::system_clock::now();
        insight_capsule.embedding = compute_embedding(insight_capsule.content);
        insight_capsule.cryptofig_blob_base64 = cryptofig_encode(insight_capsule.embedding);

        knowledgeBase.add_capsule(insight_capsule);
        LOG_DEFAULT(LogLevel::INFO, "[LearningModule] KnowledgeBase'e içgörü kapsülü eklendi: " << insight_capsule.plain_text_summary << ", ID: " << insight_capsule.id << ", Topic: " << insight_capsule.topic);
    }
}

KnowledgeBase& LearningModule::getKnowledgeBase() {
    return knowledgeBase;
}

const KnowledgeBase& LearningModule::getKnowledgeBase() const {
    return knowledgeBase;
}

IngestReport LearningModule::ingest_envelope(const Capsule& envelope, const std::string& signature, const std::string& sender_id) {
    IngestReport report;
    report.original_capsule = envelope;
    report.source_peer_id = sender_id;
    report.timestamp = std::chrono::system_clock::now();
    report.result = IngestResult::UnknownError; // Varsayılan hata

    LOG_DEFAULT(LogLevel::DEBUG, "[LearningModule] Kapsül yutma işlemi başlatıldı. ID: " << envelope.id << ", Kaynak: " << sender_id);

    try {
        // 1. İmza Doğrulama
        if (!verify_signature(envelope, signature, sender_id)) {
            report.result = IngestResult::InvalidSignature;
            report.message = "Geçersiz imza.";
            LOG_DEFAULT(LogLevel::WARNING, "[LearningModule] Kapsül yutma başarısız: Geçersiz imza. ID: " << envelope.id);
            audit_log_append(report);
            return report;
        }
        LOG_DEFAULT(LogLevel::TRACE, "[LearningModule] İmza doğrulama başarılı.");

        // 2. İçeriği Çözme (Şifre Çözme)
        Capsule decrypted_capsule = decrypt_payload(envelope);
        report.processed_capsule = decrypted_capsule; // İşlenmiş kapsülü kaydet
        LOG_DEFAULT(LogLevel::TRACE, "[LearningModule] Şifre çözme başarılı. İçerik (ilk 50 karakter): " << decrypted_capsule.content.substr(0, std::min((size_t)50, decrypted_capsule.content.length())));

        // 3. Şema Doğrulama
        if (!schema_validate(decrypted_capsule)) {
            report.result = IngestResult::SchemaMismatch;
            report.message = "Kapsül şema doğrulaması başarısız.";
            LOG_DEFAULT(LogLevel::WARNING, "[LearningModule] Kapsül yutma başarısız: Şema uyuşmazlığı. ID: " << envelope.id);
            audit_log_append(report);
            return report;
        }
        LOG_DEFAULT(LogLevel::TRACE, "[LearningModule] Şema doğrulama başarılı.");

        // 4. Unicode Temizleme
        Capsule sanitized_capsule = sanitize_unicode(decrypted_capsule);
        if (sanitized_capsule.content != decrypted_capsule.content) {
            report.result = IngestResult::SanitizationNeeded;
            report.message = "Unicode temizleme yapıldı.";
            report.processed_capsule = sanitized_capsule; // Temizlenmiş kapsülü kaydet
            LOG_DEFAULT(LogLevel::INFO, "[LearningModule] Kapsül yutma: Unicode temizleme yapıldı. ID: " << envelope.id);
        } else {
            LOG_DEFAULT(LogLevel::TRACE, "[LearningModule] Unicode temizleme gerekmedi.");
        }

        // 5. Steganografi Tespiti
        if (run_steganalysis(sanitized_capsule)) {
            report.result = IngestResult::SteganographyDetected;
            report.message = "Steganografi tespit edildi, karantinaya alınıyor.";
            // Karantinaya alma mantığı eklenebilir.
            LOG_DEFAULT(LogLevel::WARNING, "[LearningModule] Kapsül yutma başarısız: Steganografi tespit edildi. ID: " << envelope.id);
            audit_log_append(report);
            return report;
        }
        LOG_DEFAULT(LogLevel::TRACE, "[LearningModule] Steganografi tespit edilmedi.");

        // 6. Sandbox Analizi (Placeholder)
        if (!sandbox_analysis(sanitized_capsule)) {
            report.result = IngestResult::SandboxFailed;
            report.message = "Sandbox analizi başarısız oldu veya riskli içerik tespit edildi.";
            LOG_DEFAULT(LogLevel::WARNING, "[LearningModule] Kapsül yutma başarısız: Sandbox analizi. ID: " << envelope.id);
            audit_log_append(report);
            return report;
        }
        LOG_DEFAULT(LogLevel::TRACE, "[LearningModule] Sandbox analizi başarılı (Placeholder).");

        // 7. Doğrulama (Corroboration) Kontrolü (Placeholder)
        if (!corroboration_check(sanitized_capsule)) {
            report.result = IngestResult::CorroborationFailed;
            report.message = "Kapsül doğrulaması (güvenilir kaynaklarla karşılaştırma) başarısız.";
            LOG_DEFAULT(LogLevel::WARNING, "[LearningModule] Kapsül yutma başarısız: Doğrulama kontrolü. ID: " << envelope.id);
            audit_log_append(report);
            return report;
        }
        LOG_DEFAULT(LogLevel::TRACE, "[LearningModule] Doğrulama kontrolü başarılı (Placeholder).");

        // Tüm kontroller başarılı, kapsülü bilgi tabanına ekle
        knowledgeBase.add_capsule(sanitized_capsule);
        report.result = IngestResult::Success;
        report.message = "Kapsül başarıyla yutuldu ve bilgi tabanına eklendi.";
        report.processed_capsule = sanitized_capsule;
        LOG_DEFAULT(LogLevel::INFO, "[LearningModule] Kapsül başarıyla yutuldu. ID: " << envelope.id << ", Konu: " << envelope.topic);

    } catch (const std::exception& e) {
        report.result = IngestResult::UnknownError;
        report.message = "Kapsül işleme sırasında bilinmeyen bir hata oluştu: " + std::string(e.what());
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "[LearningModule] Kapsül yutma sırasında kritik hata: " << e.what() << ", ID: " << envelope.id);
    } catch (...) {
        report.result = IngestResult::UnknownError;
        report.message = "Kapsül işleme sırasında bilinmeyen bir hata oluştu.";
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "[LearningModule] Kapsül yutma sırasında bilinmeyen hata. ID: " << envelope.id);
    }

    audit_log_append(report);
    return report;
}

std::vector<float> LearningModule::compute_embedding(const std::string& text) const {
    LOG_DEFAULT(LogLevel::DEBUG, "LearningModule: compute_embedding (KnowledgeBase'den) çağrıldı.");
    // Gerçek bir embedding modeli yerine, şimdilik metnin uzunluğuna göre basit bir dummy embedding döndürelim.
    // Gelecekte NLP modülü ile entegre olacak.
    std::vector<float> embedding(CryptofigAutoencoder::INPUT_DIM);
    for (size_t i = 0; i < CryptofigAutoencoder::INPUT_DIM; ++i) {
        embedding[i] = SafeRNG::get_instance().get_float(0.0f, 1.0f); // Rastgele değerler
    }
    embedding[0] = static_cast<float>(text.length()) / 200.0f; // Basit bir özellik
    embedding[1] = static_cast<float>(std::count_if(text.begin(), text.end(), [](char c){ return std::isupper(c); })) / 50.0f;

    return embedding;
}

std::string LearningModule::cryptofig_encode(const std::vector<float>& cryptofig_vector) const {
    LOG_DEFAULT(LogLevel::DEBUG, "LearningModule: cryptofig_encode çağrıldı. Boyut: " << cryptofig_vector.size());
    // Cryptofig vektörünü Base64 string'e dönüştür.
    // float vektörünü direkt string'e çevirmek için bir mekanizma gerekiyor.
    // Basitçe her float'ı string'e çevirip birleştiriyoruz. Gelecekte daha verimli bir format kullanılabilir.
    std::ostringstream oss;
    for (float val : cryptofig_vector) {
        oss << val << " ";
    }
    return CerebrumLux::Crypto::base64_encode(oss.str());
}

std::vector<float> LearningModule::cryptofig_decode_base64(const std::string& base64_cryptofig_blob) const {
    LOG_DEFAULT(LogLevel::DEBUG, "LearningModule: cryptofig_decode_base64 çağrıldı.");
    std::string decoded_str = CerebrumLux::Crypto::base64_decode(base64_cryptofig_blob);
    std::istringstream iss(decoded_str);
    std::vector<float> cryptofig_vector;
    float val;
    while (iss >> val) {
        cryptofig_vector.push_back(val);
    }
    return cryptofig_vector;
}


bool LearningModule::verify_signature(const Capsule& capsule, const std::string& signature, const std::string& sender_id) const {
    // Gerçek imza doğrulama mantığı.
    // Şimdilik basitleştirilmiş: Eğer gönderen "Unauthorized_Peer" ise veya imza "invalid_signature_tampered" ise false dön.
    // Veya her zaman true dön (geçici olarak).
    if (sender_id == "Unauthorized_Peer" || signature == "invalid_signature_tampered") {
        LOG_DEFAULT(LogLevel::WARNING, "[LearningModule::verify_signature] Geçersiz imza veya yetkisiz gönderen tespit edildi.");
        return false;
    }
    // Gerçek kriptografik doğrulama
    try {
        std::string public_key_pem;
        // Eğer sender_id için kayıtlı bir public key varsa onu kullan
        if (!sender_id.empty()) {
            public_key_pem = cryptoManager.get_peer_public_key_pem(sender_id);
            if (public_key_pem.empty()) {
                // Peer'ın anahtarı yoksa, kendi public key'imizi kullanarak doğrulayabiliriz
                // (bu, self-signed veya trusted peer senaryosu için geçerli olabilir)
                public_key_pem = cryptoManager.get_my_public_key_pem();
            }
        } else {
             public_key_pem = cryptoManager.get_my_public_key_pem();
        }

        if (public_key_pem.empty()) {
             LOG_DEFAULT(LogLevel::WARNING, "[LearningModule::verify_signature] Doğrulama için uygun public key bulunamadı.");
             return false;
        }

        return cryptoManager.ed25519_verify(capsule.encrypted_content, signature, public_key_pem);
    } catch (const std::exception& e) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "[LearningModule::verify_signature] İmza doğrulama sırasında hata: " << e.what());
        return false;
    }
}

Capsule LearningModule::decrypt_payload(const Capsule& encrypted_capsule) const {
    // Bu, şifre çözme anahtarının nasıl yönetildiğine bağlıdır.
    // Şimdilik AES anahtarını türetmeden doğrudan dummy bir key ile çözdüğümüzü varsayalım.
    // create_signed_encrypted_capsule'daki AES anahtarı lambda içinde üretildiği için,
    // gerçek şifre çözme için o anahtara erişimimiz olmalı veya bir anahtar değişim protokolü olmalı.
    // Şimdilik, şifre çözülen içeriğin orijinal içerik olduğunu varsayalım.
    LOG_DEFAULT(LogLevel::WARNING, "[LearningModule::decrypt_payload] Gerçek şifre çözme mekanizması henüz implemente edilmedi. Şimdilik içerik orijinal haliyle kabul ediliyor.");
    Capsule decrypted_capsule = encrypted_capsule;
    decrypted_capsule.content = "Decrypted content: " + encrypted_capsule.content; // Dummy olarak işaretle
    return decrypted_capsule;
}

bool LearningModule::schema_validate(const Capsule& capsule) const {
    // Kapsül şemasının temel doğrulaması (ID, content, source, topic boş olmamalı).
    bool is_valid = !capsule.id.empty() && !capsule.content.empty() && !capsule.source.empty() && !capsule.topic.empty();
    if (!is_valid) {
        LOG_DEFAULT(LogLevel::WARNING, "[LearningModule::schema_validate] Kapsül şema doğrulaması başarısız: Eksik alanlar.");
    }
    return is_valid;
}

Capsule LearningModule::sanitize_unicode(const Capsule& capsule) const {
    Capsule sanitized_capsule = capsule;
    sanitized_capsule.content = unicodeSanitizer->sanitize(capsule.content);
    if (sanitized_capsule.content != capsule.content) {
        LOG_DEFAULT(LogLevel::DEBUG, "[LearningModule::sanitize_unicode] Kapsül içeriğinde Unicode temizleme yapıldı.");
    }
    return sanitized_capsule;
}

bool LearningModule::run_steganalysis(const Capsule& capsule) const {
    // Steganografi tespiti (placeholder)
    // Eğer içeriğinde "hidden_message_tag" gibi bir string varsa, steganografi tespit edilmiş gibi davran.
    bool detected = capsule.content.find("hidden_message_tag") != std::string::npos;
    if (detected) {
        LOG_DEFAULT(LogLevel::WARNING, "[LearningModule::run_steganalysis] Potansiyel steganografi tespit edildi.");
    }
    return detected;
}

bool LearningModule::sandbox_analysis(const Capsule& capsule) const {
    // Sandbox analizi (placeholder)
    // Her zaman true dön (güvenli olduğunu varsay).
    return true;
}

bool LearningModule::corroboration_check(const Capsule& capsule) const {
    // Doğrulama kontrolü (placeholder)
    // Her zaman true dön (doğrulandığını varsay).
    return true;
}

void LearningModule::audit_log_append(const IngestReport& report) const {
    LOG_DEFAULT(LogLevel::INFO, "[LearningModule] Denetim Kaydı: Kapsül ID: " << report.original_capsule.id
                << ", Sonuç: " << static_cast<int>(report.result)
                << ", Mesaj: " << report.message
                << ", Kaynak Peer: " << report.source_peer_id);
}

} // namespace CerebrumLux