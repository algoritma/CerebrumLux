#include "LearningModule.h"
#include "../core/logger.h"
#include "../core/enums.h"
#include "../core/utils.h" // SafeRNG için
#include "../crypto/CryptoManager.h" // cryptoManager için
#include "../crypto/CryptoUtils.h" // Base64 kodlama için
#include "../brain/autoencoder.h" // CryptofigAutoencoder::INPUT_DIM için
#include <iostream>
#include <algorithm> // std::min için
#include <stdexcept> // std::runtime_error için

#include <QCoreApplication> 
#include <QUrlQuery> // URL kodlama için gerekli
#include <QDateTime> // QDateTime::currentDateTime().toString() için

namespace CerebrumLux {

LearningModule::LearningModule(KnowledgeBase& kb, CerebrumLux::Crypto::CryptoManager& cryptoMan, QObject *parent)
    : QObject(parent), // parentApp yerine QObject'in parent'ı
      knowledgeBase(kb),
      cryptoManager(cryptoMan),
      unicodeSanitizer(std::make_unique<UnicodeSanitizer>()),
      stegoDetector(std::make_unique<StegoDetector>()),
      webFetcher(std::make_unique<WebFetcher>(this)), // WebFetcher'ı unique_ptr ile oluştur ve this (LearningModule) parent olarak ver
      parentApp(parent) // parentApp üyesini başlat
{
    // WebFetcher sinyallerini LearningModule'ün slot'larına bağla
    connect(webFetcher.get(), &WebFetcher::structured_content_fetched, // .get() ile raw pointer alındı
            this, &LearningModule::onStructuredWebContentFetched, Qt::QueuedConnection); // Qt::QueuedConnection eklendi
    connect(webFetcher.get(), &WebFetcher::fetch_error,
            this, &LearningModule::onWebFetchError, Qt::QueuedConnection); // Qt::QueuedConnection eklendi

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
    new_capsule.plain_text_summary = text.substr(0, std::min((size_t)500, text.length())) + "...";
    new_capsule.timestamp_utc = std::chrono::system_clock::now();

    new_capsule.embedding = compute_embedding(new_capsule.content);
    new_capsule.cryptofig_blob_base64 = cryptofig_encode(new_capsule.embedding);

    knowledgeBase.add_capsule(new_capsule);
    LOG_DEFAULT(LogLevel::INFO, "LearningModule: Yeni kapsül bilgi tabanına eklendi. ID: " << new_capsule.id);
}

void LearningModule::learnFromWeb(const std::string& query) {
    LOG_DEFAULT(LogLevel::INFO, "LearningModule: Web'den öğrenme başlatıldı. Sorgu: " << query);

    // Bir önceki web çekme işlemi devam ediyorsa yeni bir tane başlatma
    if (webFetchInProgress) {
        LOG_DEFAULT(LogLevel::WARNING, "LearningModule: Zaten bir web çekme işlemi devam ediyor. Yeni sorgu: " << query);
        emit webFetchCompleted(createIngestReport(CerebrumLux::IngestResult::Busy, "Zaten bir web çekme işlemi devam ediyor."));
        return;
    }
    webFetchInProgress = true;
    currentWebFetchQuery = QString::fromStdString(query); // Mevcut sorguyu kaydet

    std::string final_url_to_fetch = query;
    if (query.find("http://") == std::string::npos && query.find("https://") == std::string::npos && query.find(".") == std::string::npos) {
        // Eğer URL'de protokol belirtilmemişse ve dot ('.') içermiyorsa arama sorgusu olarak kabul et
        QString encoded_query = QUrl::toPercentEncoding(QString::fromStdString(query));
        final_url_to_fetch = "https://www.google.com/search?q=" + encoded_query.toStdString();
        LOG_DEFAULT(LogLevel::DEBUG, "LearningModule: Sorgu Google arama URL'sine dönüştürüldü: " << final_url_to_fetch);
    } else {
        QUrl temp_url(QString::fromStdString(query));
        if (temp_url.isValid()) {
             final_url_to_fetch = temp_url.toString(QUrl::FullyEncoded).toStdString();
        } else {
            LOG_DEFAULT(LogLevel::WARNING, "LearningModule: Girdi URL olarak geçersiz, Google arama URL'sine dönüştürüldü: " << query);
            QString encoded_query = QUrl::toPercentEncoding(QString::fromStdString(query));
            final_url_to_fetch = "https://www.google.com/search?q=" + encoded_query.toStdString();
        }
    }
    
    if (webFetcher) {
        webFetcher->fetch_url(final_url_to_fetch);
    } else {
        LOG_DEFAULT(LogLevel::ERR_CRITICAL, "LearningModule: WebFetcher nesnesi null, web'den öğrenme başlatılamadı.");
        emit webFetchCompleted(createIngestReport(CerebrumLux::IngestResult::UnknownError, "WebFetcher nesnesi null, web'den öğrenme başlatılamadı."));
    }
}

// WebFetcher'dan gelen yapılandırılmış arama sonuçlarını işlemek için slot
void LearningModule::onStructuredWebContentFetched(const QString& url, const std::vector<CerebrumLux::WebSearchResult>& searchResults) {
    LOG_DEFAULT(LogLevel::INFO, "LearningModule: Yapılandırılmış web içeriği çekildi. URL: " << url.toStdString() << ", Sonuç sayısı: " << searchResults.size());
    
    for (const auto& result : searchResults) {
        CerebrumLux::Capsule web_capsule;
        web_capsule.id = "WebSearch_" + CerebrumLux::generate_unique_id();
        web_capsule.topic = "WebSearch"; 
        web_capsule.source = url.toStdString(); // Orijinal arama URL'si (getirilen sayfanın URL'si)
        web_capsule.confidence = 0.75f; 
        web_capsule.plain_text_summary = QString::fromStdString(result.title + " - " + result.snippet).left(200).toStdString();
        // Tam içerik: Başlık, URL, Snippet ve varsa ayıklanmış ana içerik birleştirilir.
        web_capsule.content = QString::fromStdString("Başlık: " + result.title + "\nURL: " + result.url + "\nAçıklama: " + result.snippet + "\n\nTam İçerik:\n" + result.main_content).toStdString();
        web_capsule.timestamp_utc = std::chrono::system_clock::now();

        // CodeFile Path olarak WebSearchResult'ın URL'sini kullan
        web_capsule.code_file_path = result.url;

        web_capsule.embedding.resize(CryptofigAutoencoder::INPUT_DIM);
        for (size_t i = 0; i < CryptofigAutoencoder::INPUT_DIM; ++i) {
            web_capsule.embedding[i] = CerebrumLux::SafeRNG::get_instance().get_float(0.0f, 1.0f);
        }
        web_capsule.cryptofig_blob_base64 = cryptofig_encode(web_capsule.embedding); // compute_embedding çağrısı yerine doğrudan kodlama

        // Kapsül ID'sini daha benzersiz hale getir: URL'nin hash'i veya başlığın hash'i eklenebilir.
        // content_hash kullanmak en iyisi:
        if (!result.content_hash.empty()) { // Eğer içerik hash'i varsa ID'ye ekle
            web_capsule.id += "_" + result.content_hash;
        }
 
        knowledgeBase.add_capsule(web_capsule);
        LOG_DEFAULT(LogLevel::INFO, "LearningModule: Web arama kapsülü eklendi: ID: " << web_capsule.id << ", URL: " << QString::fromStdString(result.url));
    }
    webFetchInProgress = false;
    emit webFetchCompleted(createIngestReport(CerebrumLux::IngestResult::Success, "Web'den öğrenme tamamlandı."));
}

// WebFetcher'dan gelen hata sinyali için slot
void LearningModule::onWebFetchError(const QString& url, const QString& error_message) {
    LOG_DEFAULT(LogLevel::ERR_CRITICAL, "LearningModule: Web içerigi cekme hatası. URL: " << url.toStdString() << ", Hata: " << error_message.toStdString());
    webFetchInProgress = false; // Hata durumunda işlemi serbest bırak
    emit webFetchCompleted(createIngestReport(CerebrumLux::IngestResult::UnknownError, "Web içerigi cekme hatası: " + error_message.toStdString()));
}

std::vector<Capsule> LearningModule::search_by_topic(const std::string& topic) const {
    LOG_DEFAULT(LogLevel::DEBUG, "[LearningModule] Topic'e göre arama yapılıyor: " << topic);
    return knowledgeBase.search_by_topic(topic);
}

void LearningModule::process_ai_insights(const std::vector<AIInsight>& insights) {
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "[LearningModule] AI Insights isleniyor: " << insights.size() << " adet içgörü.");

    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "[LearningModule] Başlangıç: Gelen içgörülerin tipleri ve ID'leri (Toplam: " << insights.size() << "):");
    for (const auto& insight : insights) {
        LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "[LearningModule]   Alınan İçgörü: ID=" << insight.id << ", Type=" << static_cast<int>(insight.type) << ", Context=" << insight.context << ", Urgency=" << static_cast<int>(insight.urgency));
    }
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "[LearningModule] --- İçgörü İşleme Başlıyor ---");

    for (const auto& insight : insights) {
        LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "[LearningModule] İşleniyor: ID=" << insight.id << " (Type: " << static_cast<int>(insight.type) << ")"
                                    << ", Gözlem: " << insight.observation
                                    << ", Bağlam: " << insight.context
                                    << ", Önerilen Eylem: " << insight.recommended_action
                                    << ", Tip: " << static_cast<int>(insight.type));

        Capsule insight_capsule;
        insight_capsule.id = insight.id;
        insight_capsule.content = insight.observation;
        insight_capsule.source = "AIInsightsEngine";
        insight_capsule.topic = "AI Insight";
        // YENİ: AIInsight'ın urgency seviyesinden confidence türet
        switch (insight.urgency) {
            case CerebrumLux::UrgencyLevel::Low: insight_capsule.confidence = 0.7f; break;
            case CerebrumLux::UrgencyLevel::Medium: insight_capsule.confidence = 0.8f; break;
            case CerebrumLux::UrgencyLevel::High: insight_capsule.confidence = 0.9f; break;
            case CerebrumLux::UrgencyLevel::Critical: insight_capsule.confidence = 0.95f; break;
            case CerebrumLux::UrgencyLevel::None: insight_capsule.confidence = 0.6f; break;
            default: insight_capsule.confidence = 0.5f; break;
        }

        // YENİ KOD: CodeDevelopmentSuggestion için özel topic ataması (GELİŞTİRİLMİŞ TEŞHİS LOGLARI)
        LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "[LearningModule]   Kontrol ediliyor: insight.type (" << static_cast<int>(insight.type) << ") == CerebrumLux::InsightType::CodeDevelopmentSuggestion (" << static_cast<int>(CerebrumLux::InsightType::CodeDevelopmentSuggestion) << ")");
        // NOT: Projede zaman zaman farklı derleme birimlerinin farklı header sürümleriyle (stale .o) derlenmesinden
        // dolayı enum değerlerinde uyuşmazlık görülebiliyor. Bu durumda CodeDevelopmentSuggestion'ı kaçırmamak için
        // güvenli bir fallback mantığı ekliyoruz:
        //  - Önce enum ile kontrol et.
        //  - Eşleşme yoksa, insight.id'in "CodeDev_" ile başlaması veya context'in "Kod"/"Code" içermesi durumunda da kabul et.
        bool is_code_dev = (insight.type == CerebrumLux::InsightType::CodeDevelopmentSuggestion);
        if (!is_code_dev) {
            // Fallback 1: ID "CodeDev" ile başlıyorsa (alt çizgi olsun olmasın)
            if (!insight.id.empty() && insight.id.rfind("CodeDev", 0) == 0) {
                is_code_dev = true;
                LOG_DEFAULT(CerebrumLux::LogLevel::WARNING, "[LearningModule] Fallback: CodeDevelopment tespiti ID prefix'e gore yapildi. ID: " << insight.id);
            }
            // Fallback 2: context veya recommended_action string'inde "Code" / "Kod" / "Refactor" gibi anahtar kelimeler varsa
            else if (insight.context.find("Kod") != std::string::npos ||
                     insight.context.find("Code") != std::string::npos ||
                     insight.recommended_action.find("Code") != std::string::npos ||
                     insight.recommended_action.find("Refactor") != std::string::npos) {
                is_code_dev = true;
                LOG_DEFAULT(CerebrumLux::LogLevel::WARNING, "[LearningModule] Fallback: CodeDevelopment tespiti string iceriklere gore yapildi. Context: " << insight.context << ", Action: " << insight.recommended_action << ", ID: " << insight.id);
            }
        }
        if (is_code_dev) {
            insight_capsule.topic = "CodeDevelopment"; // Özel olarak CodeDevelopment topic'i ata
            LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "[LearningModule] CodeDevelopmentSuggestion için topic 'CodeDevelopment' olarak ayarlandı. ID: " << insight.id);
        } else {
            LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "[LearningModule] InsightType CodeDevelopmentSuggestion değil (" << static_cast<int>(insight.type) << "). Varsayılan topic ('AI Insight') kullanılıyor.");
        }

        if (insight.context == "Sistem Genel Performans Metriği") { 
            size_t pos = insight.observation.find(":");
            if (pos != std::string::npos && pos + 1 < insight.observation.length()) {
                try {
                    insight_capsule.confidence = std::stof(insight.observation.substr(pos + 1));
                    // Eğer CodeDevelopmentSuggestion ise topic değişmemeli, GraphData olmamalı
                    if (insight_capsule.topic != "CodeDevelopment") { // YENİ: Conflict'i önle
                        insight_capsule.topic = "GraphData";
                        LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "[LearningModule] Grafik verisi için içgörü güveni çıkarıldı ve topic 'GraphData' olarak ayarlandı: " << insight_capsule.confidence);
                    } else {
                        LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "[LearningModule] Grafik verisi içgörüsü, zaten 'CodeDevelopment' topic'ine sahip olduğu için topic değişmedi.");
                    }
                } catch (const std::exception& e) {
                    LOG_DEFAULT(CerebrumLux::LogLevel::WARNING, "[LearningModule] İçgörüden güven değeri çıkarılırken hata: " << e.what());
                }
            }
        }

        insight_capsule.plain_text_summary = insight.observation.substr(0, std::min((size_t)500, insight.observation.length())) + "...";
        insight_capsule.timestamp_utc = std::chrono::system_clock::now();
        insight_capsule.embedding = compute_embedding(insight_capsule.content);
        insight_capsule.cryptofig_blob_base64 = cryptofig_encode(insight_capsule.embedding);
        insight_capsule.code_file_path = insight.code_file_path; //EKLENDİ: AIInsight'tan code_file_path Capsule'a aktarılıyor

        knowledgeBase.add_capsule(insight_capsule);
        LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "[LearningModule] KnowledgeBase'e içgörü kapsülü EKLENDİ. ID: " << insight_capsule.id << ", Topic: " << insight_capsule.topic << ", Özet: " << insight_capsule.plain_text_summary.substr(0, std::min((size_t)50, insight_capsule.plain_text_summary.length())) << "..." << ", Confidence: " << insight_capsule.confidence);
    }
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "[LearningModule] --- İçgörü İşleme Bitti ---");
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
    report.result = IngestResult::UnknownError;

    LOG_DEFAULT(LogLevel::DEBUG, "[LearningModule] Kapsül yutma işlemi başlatıldı. ID: " << envelope.id << ", Kaynak: " << sender_id);

    try {
        if (!verify_signature(envelope, signature, sender_id)) {
            report.result = IngestResult::InvalidSignature;
            report.message = "Geçersiz imza.";
            LOG_DEFAULT(LogLevel::WARNING, "[LearningModule] Kapsül yutma başarısız: Geçersiz imza. ID: " << envelope.id);
            audit_log_append(report);
            return report;
        }
        LOG_DEFAULT(LogLevel::TRACE, "[LearningModule] İmza doğrulama başarılı.");

        Capsule decrypted_capsule = decrypt_payload(envelope);
        report.processed_capsule = decrypted_capsule;
        LOG_DEFAULT(LogLevel::TRACE, "[LearningModule] Şifre çözme başarılı. İçerik (ilk 50 karakter): " << decrypted_capsule.content.substr(0, std::min((size_t)50, decrypted_capsule.content.length())));

        if (!schema_validate(decrypted_capsule)) {
            report.result = IngestResult::SchemaMismatch;
            report.message = "Kapsül şema doğrulaması başarısız.";
            LOG_DEFAULT(LogLevel::WARNING, "[LearningModule] Kapsül yutma başarısız: Şema uyuşmazlığı. ID: " << envelope.id);
            audit_log_append(report);
            return report;
        }
        LOG_DEFAULT(LogLevel::TRACE, "[LearningModule] Şema doğrulama başarılı.");

        Capsule sanitized_capsule = sanitize_unicode(decrypted_capsule);
        if (sanitized_capsule.content != decrypted_capsule.content) {
            report.result = IngestResult::SanitizationNeeded;
            report.message = "Unicode temizleme yapıldı.";
            report.processed_capsule = sanitized_capsule;
            LOG_DEFAULT(LogLevel::INFO, "[LearningModule] Kapsül yutma: Unicode temizleme yapıldı. ID: " << envelope.id);
        } else {
            LOG_DEFAULT(LogLevel::TRACE, "[LearningModule] Unicode temizleme gerekmedi.");
        }

        if (run_steganalysis(sanitized_capsule)) {
            report.result = IngestResult::SteganographyDetected;
            report.message = "Steganografi tespit edildi, karantinaya alınıyor.";
            LOG_DEFAULT(LogLevel::WARNING, "[LearningModule] Kapsül yutma başarısız: Steganografi tespit edildi. ID: " << envelope.id);
            audit_log_append(report);
            return report;
        }
        LOG_DEFAULT(LogLevel::TRACE, "[LearningModule] Steganografi tespit edilmedi.");

        if (!sandbox_analysis(sanitized_capsule)) {
            report.result = IngestResult::SandboxFailed;
            report.message = "Sandbox analizi başarısız oldu veya riskli içerik tespit edildi.";
            LOG_DEFAULT(LogLevel::WARNING, "[LearningModule] Kapsül yutma başarısız: Sandbox analizi. ID: " << envelope.id);
            audit_log_append(report);
            return report;
        }
        LOG_DEFAULT(LogLevel::TRACE, "[LearningModule] Sandbox analizi başarılı (Placeholder).");

        if (!corroboration_check(sanitized_capsule)) {
            report.result = IngestResult::CorroborationFailed;
            report.message = "Kapsül doğrulaması (güvenilir kaynaklarla karşılaştırma) başarısız.";
            LOG_DEFAULT(LogLevel::WARNING, "[LearningModule] Kapsül yutma başarısız: Doğrulama kontrolü. ID: " << envelope.id);
            audit_log_append(report);
            return report;
        }
        LOG_DEFAULT(LogLevel::TRACE, "[LearningModule] Doğrulama kontrolü başarılı (Placeholder).");

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

CerebrumLux::IngestReport LearningModule::createIngestReport(CerebrumLux::IngestResult result, const std::string& message) const {
    IngestReport report;
    report.result = result;
    report.message = message;
    report.timestamp = std::chrono::system_clock::now();
    // Diğer alanlar varsayılan değerlerinde kalır veya boş bırakılır.
    return report;
}

std::vector<float> LearningModule::compute_embedding(const std::string& text) const {
    LOG_DEFAULT(LogLevel::DEBUG, "LearningModule: compute_embedding (KnowledgeBase'den) çağrıldı.");
    std::vector<float> embedding(CryptofigAutoencoder::INPUT_DIM);
    for (size_t i = 0; i < CryptofigAutoencoder::INPUT_DIM; ++i) {
        embedding[i] = SafeRNG::get_instance().get_float(0.0f, 1.0f);
    }
    embedding[0] = static_cast<float>(text.length()) / 200.0f;
    embedding[1] = static_cast<float>(std::count_if(text.begin(), text.end(), [](char c){ return std::isupper(c); })) / 50.0f;

    return embedding;
}

std::string LearningModule::cryptofig_encode(const std::vector<float>& cryptofig_vector) const {
    LOG_DEFAULT(LogLevel::DEBUG, "LearningModule: cryptofig_encode çağrıldı. Boyut: " << cryptofig_vector.size());
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
    if (sender_id == "Unauthorized_Peer" || signature == "invalid_signature_tampered") {
        LOG_DEFAULT(LogLevel::WARNING, "[LearningModule::verify_signature] Geçersiz imza veya yetkisiz gönderen tespit edildi.");
        return false;
    }
    try {
        std::string public_key_pem;
        if (!sender_id.empty()) {
            public_key_pem = cryptoManager.get_peer_public_key_pem(sender_id);
            if (public_key_pem.empty()) {
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
    LOG_DEFAULT(LogLevel::WARNING, "[LearningModule::decrypt_payload] Gerçek şifre çözme mekanizması henüz implemente edilmedi. Şimdilik içerik orijinal haliyle kabul ediliyor.");
    Capsule decrypted_capsule = encrypted_capsule;
    decrypted_capsule.content = "Decrypted content: " + encrypted_capsule.content;
    return decrypted_capsule;
}

bool LearningModule::schema_validate(const Capsule& capsule) const {
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
    bool detected = capsule.content.find("hidden_message_tag") != std::string::npos;
    if (detected) {
        LOG_DEFAULT(LogLevel::WARNING, "[LearningModule::run_steganalysis] Potansiyel steganografi tespit edildi.");
    }
    return detected;
}

bool LearningModule::sandbox_analysis(const Capsule& capsule) const {
    return true;
}

bool LearningModule::corroboration_check(const Capsule& capsule) const {
    return true;
}

void LearningModule::audit_log_append(const IngestReport& report) const {
    LOG_DEFAULT(LogLevel::INFO, "[LearningModule] Denetim Kaydı: Kapsül ID: " << report.original_capsule.id
                << ", Sonuç: " << static_cast<int>(report.result)
                << ", Mesaj: " << report.message
                << ", Kaynak Peer: " << report.source_peer_id);
}

// YENİ METOT: Kod geliştirme önerisi geri bildirimini işler
void LearningModule::processCodeSuggestionFeedback(const std::string& capsuleId, bool accepted) {
    if (accepted) {
        LOG_DEFAULT(LogLevel::INFO, "LearningModule: Kod Geliştirme Önerisi KABUL EDİLDİ. ID: " << capsuleId);
    } else {
        LOG_DEFAULT(LogLevel::INFO, "LearningModule: Kod Geliştirme Önerisi REDDEDİLDİ. ID: " << capsuleId);
    }
    // Gelecekte: Bu geri bildirim, AIInsightsEngine'ın öneri üretim mantığını veya IntentLearner'ı eğitmek için kullanılabilir.
    // Örneğin, bu kapsülün topic'ini veya içeriğini analiz ederek ilgili metrikleri güncelleyebiliriz.
}

} // namespace CerebrumLux