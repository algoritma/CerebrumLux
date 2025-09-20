#include "LearningModule.h"
#include <iostream>
#include <algorithm> // std::min için
#include "../core/logger.h" // Loglama için
#include "../learning/WebFetcher.h" 

// YENİ: UnicodeSanitizer ve StegoDetector başlık dosyaları
#include "UnicodeSanitizer.h"
#include "StegoDetector.h"


// Kurucu
LearningModule::LearningModule(KnowledgeBase& kb) 
    : knowledgeBase(kb),
      unicodeSanitizer(std::make_unique<UnicodeSanitizer>()), // YENİ: Modülleri başlat
      stegoDetector(std::make_unique<StegoDetector>())      // YENİ
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
    static int text_id_counter = 1000; 
    c.id = ++text_id_counter; 

    c.topic = topic;
    c.source = source;
    c.content = text; 
    c.encrypted_content = knowledgeBase.encrypt(text); 
    c.confidence = confidence;

    knowledgeBase.addCapsule(c);
    knowledgeBase.save(); 
    LOG_DEFAULT(LogLevel::INFO, "LearningModule: Learned from text. Topic: " << topic);
}

void LearningModule::learnFromWeb(const std::string& query) {
    WebFetcher fetcher;
    auto results = fetcher.search(query);

    for (auto& r : results) {
        learnFromText(r.content, r.source, query, 0.9f);
    }
    LOG_DEFAULT(LogLevel::INFO, "LearningModule: Learned from web query: " << query);
}

void LearningModule::process_ai_insights(const std::vector<AIInsight>& insights) {
    LOG_DEFAULT(LogLevel::INFO, "[LearningModule] AI Insights isleniyor: " << insights.size() << " adet içgörü.");
    for (const auto& insight : insights) {
        Capsule c;
        static int insight_id_counter = 0;
        c.id = ++insight_id_counter; 
        c.content = insight.observation; 
        c.source = "AIInsightsEngine";
        c.topic = "AI Insight"; 
        c.confidence = insight.urgency; 
        
        c.encrypted_content = knowledgeBase.encrypt(c.content); 

        knowledgeBase.addCapsule(c);
        LOG_DEFAULT(LogLevel::INFO, "[LearningModule] KnowledgeBase'e içgörü kapsülü eklendi: " << c.content.substr(0, std::min((size_t)30, c.content.length())) << "...");
    }
    knowledgeBase.save(); 
}

// YENİ: Güvenli kapsül işleme pipeline'ı implementasyonu (iskelet)
IngestReport LearningModule::ingest_envelope(const Capsule& envelope, const std::string& signature, const std::string& sender_id) {
    IngestReport report;
    report.original_capsule = envelope;
    report.timestamp = std::chrono::system_clock::now();
    report.source_peer_id = sender_id;

    LOG_DEFAULT(LogLevel::INFO, "LearningModule: Ingesting envelope from " << sender_id << "...");

    // 1. İmza Doğrulama
    if (!verify_signature(envelope, signature, sender_id)) {
        report.result = IngestResult::InvalidSignature;
        report.message = "Signature verification failed.";
        audit_log_append(report);
        return report;
    }
    LOG_DEFAULT(LogLevel::DEBUG, "LearningModule: Signature verified.");

    // 2. Yükü Şifre Çözme
    Capsule decrypted_capsule = decrypt_payload(envelope);
    if (decrypted_capsule.content.empty()) { // Örnek hata kontrolü
        report.result = IngestResult::DecryptionFailed;
        report.message = "Payload decryption failed.";
        audit_log_append(report);
        return report;
    }
    report.processed_capsule = decrypted_capsule; // İşlemeye devam etmek için şifresi çözülmüş kapsülü kullan
    LOG_DEFAULT(LogLevel::DEBUG, "LearningModule: Payload decrypted.");

    // 3. Şema Doğrulama
    if (!schema_validate(report.processed_capsule)) {
        report.result = IngestResult::SchemaMismatch;
        report.message = "Capsule schema mismatch.";
        audit_log_append(report);
        return report;
    }
    LOG_DEFAULT(LogLevel::DEBUG, "LearningModule: Schema validated.");

    // 4. Unicode Temizleme
    Capsule sanitized_capsule = sanitize_unicode(report.processed_capsule);
    if (sanitized_capsule.content != report.processed_capsule.content) { // Değişiklik olduysa
        report.result = IngestResult::SanitizationNeeded; // Sadece bilgi amaçlı
        report.message = "Unicode sanitization applied.";
        report.processed_capsule = sanitized_capsule; // Temizlenmiş kapsülü kullan
    }
    LOG_DEFAULT(LogLevel::DEBUG, "LearningModule: Unicode sanitized.");

    // 5. Steganografi Analizi
    if (run_steganalysis(report.processed_capsule)) {
        report.result = IngestResult::SteganographyDetected;
        report.message = "Steganography detected in capsule content.";
        audit_log_append(report);
        return report;
    }
    LOG_DEFAULT(LogLevel::DEBUG, "LearningModule: Steganography check passed.");

    // 6. Sandbox Analizi (Harici bir sürece veya sanal ortama gönderme simülasyonu)
    if (!sandbox_analysis(report.processed_capsule)) {
        report.result = IngestResult::SandboxFailed;
        report.message = "Sandbox analysis indicated potential threat.";
        audit_log_append(report);
        return report;
    }
    LOG_DEFAULT(LogLevel::DEBUG, "LearningModule: Sandbox analysis passed.");

    // 7. Doğrulama (Corroboration) Çek (Bilgi Tabanı ile Çapraz Kontrol)
    if (!corroboration_check(report.processed_capsule)) {
        report.result = IngestResult::CorroborationFailed;
        report.message = "Corroboration check failed against existing knowledge.";
        audit_log_append(report);
        return report;
    }
    LOG_DEFAULT(LogLevel::DEBUG, "LearningModule: Corroboration check passed.");

    // Tüm kontroller başarılı, kapsülü KnowledgeBase'e ekle
    knowledgeBase.addCapsule(report.processed_capsule);
    knowledgeBase.save(); // Her başarılı eklemeden sonra kaydet
    report.result = IngestResult::Success;
    report.message = "Capsule ingested successfully.";
    audit_log_append(report);
    LOG_DEFAULT(LogLevel::INFO, "LearningModule: Capsule ingested successfully from " << sender_id);
    return report;
}

// YENİ: Yardımcı metodların iskelet implementasyonları
bool LearningModule::verify_signature(const Capsule& capsule, const std::string& signature, const std::string& sender_id) {
    // Gerçek bir implementasyonda, kriptografik imza doğrulama mantığı buraya gelir.
    // Şimdilik, sadece imzanın boş olmadığını kontrol edelim.
    if (signature.empty() && sender_id != "Self") { // Kendimizden gelmiyorsa imza olmalı
        LOG_DEFAULT(LogLevel::WARNING, "LearningModule: Signature missing for capsule from " << sender_id);
        return false;
    }
    // Basit doğrulama: "valid_signature" string'i kontrol et
    return signature == "valid_signature" || sender_id == "Self";
}

Capsule LearningModule::decrypt_payload(const Capsule& encrypted_capsule) {
    // Gerçek bir implementasyonda, kriptografik şifre çözme mantığı buraya gelir.
    // Şimdilik, knowledgeBase'in encrypt/decrypt metodunu kullanalım.
    Capsule decrypted = encrypted_capsule;
    try {
        decrypted.content = knowledgeBase.encrypt(encrypted_capsule.encrypted_content); // XOR'u tersine çevirmek için tekrar XOR
        if (decrypted.content == encrypted_capsule.encrypted_content && !encrypted_capsule.encrypted_content.empty()) {
             // Eğer şifre çözme sonrası aynı kaldıysa ve boş değilse, belki de şifre çözülmedi veya zaten şifresizdi
             // Basitçe: şifresiz içerik boşsa ve şifreli içerik doluysa, şifre çözme başarısız varsayalım.
             // Daha iyi bir yaklaşım, şifrelenmiş içeriğin formatını kontrol etmektir.
            LOG_DEFAULT(LogLevel::WARNING, "LearningModule: Decryption might have failed or content was not encrypted for capsule id " << encrypted_capsule.id);
            // Geçici olarak, şifreli_content'i doğrudan content'e kopyalayıp şifre çözme başarısız oldu diyelim.
            // Bu kısım gerçek bir kripto sisteminde çok daha sağlam olmalı.
            decrypted.content.clear(); // Şifre çözme başarısız olduysa içeriği boşalt
            return decrypted;
        }
    } catch (const std::exception& e) {
        LOG_DEFAULT(LogLevel::ERR_CRITICAL, "LearningModule: Exception during decryption: " << e.what());
        decrypted.content.clear();
        return decrypted;
    }
    return decrypted;
}

bool LearningModule::schema_validate(const Capsule& capsule) {
    // Gerçek bir implementasyonda, kapsülün JSON şemasına uygunluğunu kontrol eden mantık buraya gelir.
    // Şimdilik, gerekli alanların (id, content, source, topic) boş olmadığını kontrol edelim.
    bool valid = capsule.id != 0 && !capsule.content.empty() && !capsule.source.empty() && !capsule.topic.empty();
    if (!valid) {
        LOG_DEFAULT(LogLevel::WARNING, "LearningModule: Schema validation failed for capsule id " << capsule.id);
    }
    return valid;
}

Capsule LearningModule::sanitize_unicode(const Capsule& capsule) {
    // Gerçek bir implementasyonda, UnicodeSanitizer sınıfı burada kullanılır.
    // Şimdilik, basitçe yeni oluşturulan UnicodeSanitizer'ı kullanalım.
    Capsule sanitized_capsule = capsule;
    if (unicodeSanitizer) {
        sanitized_capsule.content = unicodeSanitizer->sanitize(capsule.content);
        if (sanitized_capsule.content != capsule.content) {
            LOG_DEFAULT(LogLevel::INFO, "LearningModule: Unicode sanitization changed content for capsule id " << capsule.id);
        }
    } else {
        LOG_DEFAULT(LogLevel::WARNING, "LearningModule: UnicodeSanitizer not available for sanitization.");
    }
    return sanitized_capsule;
}

bool LearningModule::run_steganalysis(const Capsule& capsule) {
    // Gerçek bir implementasyonda, StegoDetector sınıfı burada kullanılır.
    // Şimdilik, basitçe yeni oluşturulan StegoDetector'ı kullanalım.
    if (stegoDetector) {
        if (stegoDetector->detectSteganography(capsule.content)) {
            LOG_DEFAULT(LogLevel::WARNING, "LearningModule: Steganography detected in capsule id " << capsule.id);
            return true;
        }
    } else {
        LOG_DEFAULT(LogLevel::WARNING, "LearningModule: StegoDetector not available for steganalysis.");
    }
    // Basit heuristik: belirli anahtar kelimeleri arayalım
    if (capsule.content.find("hidden_data") != std::string::npos ||
        capsule.content.find("stego_marker") != std::string::npos) {
        LOG_DEFAULT(LogLevel::WARNING, "LearningModule: Simple steganography heuristic triggered for capsule id " << capsule.id);
        return true;
    }
    return false;
}

bool LearningModule::sandbox_analysis(const Capsule& capsule) {
    // Gerçek bir implementasyonda, kapsül içeriği güvenli bir sanal ortamda analiz edilir.
    // Örneğin, potansiyel olarak kötü amaçlı kod veya veri olup olmadığı kontrol edilir.
    // Şimdilik, sadece belirli anahtar kelimeleri arayarak basit bir simülasyon yapalım.
    if (capsule.content.find("malware_signature") != std::string::npos ||
        capsule.content.find("exploit_code") != std::string::npos) {
        LOG_DEFAULT(LogLevel::ERR_CRITICAL, "LearningModule: Sandbox analysis detected potential threat in capsule id " << capsule.id);
        return false; // Tehdit algılandı
    }
    return true; // Güvenli varsayalım
}

bool LearningModule::corroboration_check(const Capsule& capsule) {
    // Gerçek bir implementasyonda, yeni gelen kapsülün bilgileri mevcut KnowledgeBase ile çapraz kontrol edilir.
    // Bilgilerin tutarlılığı, doğruluğu ve tekrarlayıcılığı değerlendirilir.
    // Şimdilik, çok benzer bir kapsülün zaten mevcut olup olmadığını kontrol edelim.
    auto similar_capsules = knowledgeBase.findSimilar(capsule.content, 1);
    if (!similar_capsules.empty() && similar_capsules[0].confidence > 0.95f) { // Yüksek benzerlik varsa
        LOG_DEFAULT(LogLevel::WARNING, "LearningModule: Corroboration check found highly similar existing knowledge for capsule id " << capsule.id);
        // Bu bir hata değil, sadece bilgi. Kapsülü yine de kabul edebiliriz ama bir uyarı logu düşer.
    }
    return true; // Şimdilik her zaman doğrulanmış sayalım
}

void LearningModule::audit_log_append(const IngestReport& report) {
    // Gerçek bir implementasyonda, bu rapor bir denetim günlüğü sistemine kaydedilir.
    // Bu, güvenlik ve izlenebilirlik için önemlidir.
    // Şimdilik, sadece loglayalım.
    LOG_DEFAULT(LogLevel::WARNING, "LearningModule: Ingest Audit Report - Result: " << static_cast<int>(report.result) 
                                      << ", Message: " << report.message 
                                      << ", Source Peer: " << report.source_peer_id 
                                      << ", Original Capsule ID: " << report.original_capsule.id);
}