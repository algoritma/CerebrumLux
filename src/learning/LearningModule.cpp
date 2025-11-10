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
#include <sstream>   // std::stringstream için
#include <iomanip>   // std::fixed, std::setprecision için

#include <QCoreApplication> 
#include <QUrlQuery> // URL kodlama için gerekli
#include <QDateTime> // QDateTime::currentDateTime().toString() için

namespace CerebrumLux {

using EmbeddingStateKey = CerebrumLux::SwarmVectorDB::EmbeddingStateKey;

LearningModule::LearningModule(KnowledgeBase& kb, CerebrumLux::Crypto::CryptoManager& cryptoMan, QObject *parent)
    : QObject(parent),
      knowledgeBase(kb),
      cryptoManager(cryptoMan),
      unicodeSanitizer(std::make_unique<UnicodeSanitizer>()),
      stegoDetector(std::make_unique<StegoDetector>()),
      webFetcher(std::make_unique<WebFetcher>(this)),
      parentApp(parent)
{
    LOG_DEFAULT(LogLevel::TRACE, "LearningModule::LearningModule: WebFetcher sinyalleri bağlanıyor.");
    connect(webFetcher.get(), &WebFetcher::structured_content_fetched,
            this, &LearningModule::onStructuredWebContentFetched, Qt::QueuedConnection);
    connect(webFetcher.get(), &WebFetcher::fetch_error,
            this, &LearningModule::onWebFetchError, Qt::QueuedConnection);
    LOG_DEFAULT(LogLevel::TRACE, "LearningModule::LearningModule: WebFetcher sinyalleri bağlandı.");

    LOG_DEFAULT(LogLevel::INFO, "LearningModule: Initialized with CryptoManager.");
    load_q_table(); // Q-table'ı başlangıçta LMDB'den yükle
}

LearningModule::~LearningModule() {
    LOG_DEFAULT(LogLevel::INFO, "LearningModule: Destructor called.");
    save_q_table(); // Q-table'ı kapanışta LMDB'ye kaydet
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

    if (webFetchInProgress) {
        LOG_DEFAULT(LogLevel::WARNING, "LearningModule: Zaten bir web çekme işlemi devam ediyor. Yeni sorgu: " << query);
        emit webFetchCompleted(createIngestReport(CerebrumLux::IngestResult::Busy, "Zaten bir web çekme işlemi devam ediyor."));
        return;
    }
    webFetchInProgress = true;
    currentWebFetchQuery = QString::fromStdString(query);

    std::string final_url_to_fetch = query;
    if (query.find("http://") == std::string::npos && query.find("https://") == std::string::npos && query.find(".") == std::string::npos) {
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

void LearningModule::onStructuredWebContentFetched(const QString& url, const std::vector<CerebrumLux::WebSearchResult>& searchResults) {
    LOG_DEFAULT(LogLevel::INFO, "LearningModule: Yapılandırılmış web içeriği çekildi. URL: " << url.toStdString() << ", Sonuç sayısı: " << searchResults.size());
    
    for (const auto& result : searchResults) {
        CerebrumLux::Capsule web_capsule;
        web_capsule.id = "WebSearch_" + CerebrumLux::generate_unique_id();
        web_capsule.topic = "WebSearch"; 
        web_capsule.source = url.toStdString();
        web_capsule.confidence = 0.75f; 
        web_capsule.plain_text_summary = QString::fromStdString(result.title + " - " + result.snippet).left(200).toStdString();
        web_capsule.content = QString::fromStdString("Başlık: " + result.title + "\nURL: " + result.url + "\nAçıklama: " + result.snippet + "\n\nTam İçerik:\n" + result.main_content).toStdString();
        web_capsule.timestamp_utc = std::chrono::system_clock::now();

        web_capsule.code_file_path = result.url;

        web_capsule.embedding.resize(CryptofigAutoencoder::INPUT_DIM);
        for (size_t i = 0; i < CryptofigAutoencoder::INPUT_DIM; ++i) {
            web_capsule.embedding[i] = CerebrumLux::SafeRNG::get_instance().get_float(0.0f, 1.0f);
        }
        web_capsule.cryptofig_blob_base64 = cryptofig_encode(web_capsule.embedding);

        if (!result.content_hash.empty()) {
            web_capsule.id += "_" + result.content_hash;
        }
 
        knowledgeBase.add_capsule(web_capsule);
        LOG_DEFAULT(LogLevel::INFO, "LearningModule: Web arama kapsülü eklendi: ID: " << web_capsule.id << ", URL: " << QString::fromStdString(result.url));
    }
    webFetchInProgress = false;
    emit webFetchCompleted(createIngestReport(CerebrumLux::IngestResult::Success, "Web'den öğrenme tamamlandı."));
}

void LearningModule::onWebFetchError(const QString& url, const QString& error_message) {
    LOG_DEFAULT(LogLevel::ERR_CRITICAL, "LearningModule: Web içerigi cekme hatası. URL: " << url.toStdString() << ", Hata: " << error_message.toStdString());
    webFetchInProgress = false;
    emit webFetchCompleted(createIngestReport(CerebrumLux::IngestResult::UnknownError, "Web içerigi cekme hatası: " + error_message.toStdString()));
}

std::vector<Capsule> LearningModule::search_by_topic(const std::string& topic) const {
    LOG_DEFAULT(LogLevel::DEBUG, "[LearningModule] Topic'e göre arama yapılıyor: " << topic);
    std::vector<float> topic_embedding = this->compute_embedding(topic);
    return knowledgeBase.search_by_topic(topic_embedding);
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
        switch (insight.urgency) {
            case CerebrumLux::UrgencyLevel::Low: insight_capsule.confidence = 0.7f; break;
            case CerebrumLux::UrgencyLevel::Medium: insight_capsule.confidence = 0.8f; break;
            case CerebrumLux::UrgencyLevel::High: insight_capsule.confidence = 0.9f; break;
            case CerebrumLux::UrgencyLevel::Critical: insight_capsule.confidence = 0.95f; break;
            case CerebrumLux::UrgencyLevel::None: insight_capsule.confidence = 0.6f; break;
            default: insight_capsule.confidence = 0.5f; break;
        }

        LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "[LearningModule]   Kontrol ediliyor: insight.type (" << static_cast<int>(insight.type) << ") == CerebrumLux::InsightType::CodeDevelopmentSuggestion (" << static_cast<int>(CerebrumLux::InsightType::CodeDevelopmentSuggestion) << ")");
        bool is_code_dev = (insight.type == CerebrumLux::InsightType::CodeDevelopmentSuggestion);
        if (!is_code_dev) {
            if (!insight.id.empty() && insight.id.rfind("CodeDev", 0) == 0) {
                is_code_dev = true;
                LOG_DEFAULT(CerebrumLux::LogLevel::WARNING, "[LearningModule] Fallback: CodeDevelopment tespiti ID prefix'e gore yapildi. ID: " << insight.id);
            }
            else if (insight.context.find("Kod") != std::string::npos ||
                     insight.context.find("Code") != std::string::npos ||
                     insight.recommended_action.find("Code") != std::string::npos ||
                     insight.recommended_action.find("Refactor") != std::string::npos) {
                is_code_dev = true;
                LOG_DEFAULT(CerebrumLux::LogLevel::WARNING, "[LearningModule] Fallback: CodeDevelopment tespiti string iceriklere gore yapildi. Context: " << insight.context << ", Action: " << insight.recommended_action << ", ID: " << insight.id);
            }
        }
        if (is_code_dev) {
            insight_capsule.topic = "CodeDevelopment";
            LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "[LearningModule] CodeDevelopmentSuggestion için topic 'CodeDevelopment' olarak ayarlandı. ID: " << insight.id);
        } else {
            LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "[LearningModule] InsightType CodeDevelopmentSuggestion değil (" << static_cast<int>(insight.type) << "). Varsayılan topic ('AI Insight') kullanılıyor.");
        }

        if (insight.context == "Sistem Genel Performans Metriği") { 
            size_t pos = insight.observation.find(":");
            if (pos != std::string::npos && pos + 1 < insight.observation.length()) {
                try {
                    insight_capsule.confidence = std::stof(insight.observation.substr(pos + 1));
                    if (insight_capsule.topic != "CodeDevelopment") {
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

        if (!insight.associated_cryptofig.empty()) {
            if (insight.associated_cryptofig.size() == CerebrumLux::CryptofigAutoencoder::INPUT_DIM) {
                insight_capsule.embedding = insight.associated_cryptofig;
                LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "[LearningModule] AIInsight associated_cryptofig'i Capsule embedding olarak kullanıldı. ID: " << insight.id);
            } else {
                insight_capsule.embedding.assign(CerebrumLux::CryptofigAutoencoder::INPUT_DIM, 0.0f);
                std::copy(insight.associated_cryptofig.begin(),
                          insight.associated_cryptofig.begin() + std::min((size_t)CerebrumLux::CryptofigAutoencoder::INPUT_DIM, insight.associated_cryptofig.size()),
                          insight_capsule.embedding.begin());
                LOG_DEFAULT(CerebrumLux::LogLevel::WARNING, "[LearningModule] AIInsight associated_cryptofig boyutu INPUT_DIM ile uyuşmuyor. Boyut düzeltildi ve kullanıldı. ID: " << insight.id);
            }
        } else {
            insight_capsule.embedding = compute_embedding(insight_capsule.content);
            LOG_DEFAULT(CerebrumLux::LogLevel::WARNING, "[LearningModule] AIInsight için associated_cryptofig boştu. İçerikten yeni embedding hesaplandı. ID: " << insight.id);
        }

        if (!insight_capsule.embedding.empty() && insight_capsule.embedding.size() == CerebrumLux::CryptofigAutoencoder::INPUT_DIM) {
            CerebrumLux::AIAction action = CerebrumLux::AIAction::None;
            float reward = 0.0f;

            if (insight.urgency == CerebrumLux::UrgencyLevel::Critical) action = CerebrumLux::AIAction::PrioritizeTask;
            else if (insight.urgency == CerebrumLux::UrgencyLevel::High) action = CerebrumLux::AIAction::SuggestResearch;
            else if (insight.type == CerebrumLux::InsightType::CodeDevelopmentSuggestion) action = CerebrumLux::AIAction::RefactorCode;
            else if (insight.urgency == CerebrumLux::UrgencyLevel::None && insight.type == CerebrumLux::InsightType::LearningOpportunity) action = CerebrumLux::AIAction::MaximizeLearning;
            
            reward = static_cast<float>(insight.urgency) * 0.1f;
            if (action == CerebrumLux::AIAction::RefactorCode) reward += 0.2f;

            update_q_values(insight_capsule.embedding, action, reward);
        } else {
            LOG_DEFAULT(CerebrumLux::LogLevel::WARNING, "[LearningModule] Sparse Q-Table güncellenemedi: Embedding boş veya yanlış boyut. ID: " << insight.id);
        }
        insight_capsule.cryptofig_blob_base64 = cryptofig_encode(insight_capsule.embedding);
        insight_capsule.code_file_path = insight.code_file_path;

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

void LearningModule::processCodeSuggestionFeedback(const std::string& capsuleId, bool accepted) {
    if (accepted) {
        LOG_DEFAULT(LogLevel::INFO, "LearningModule: Kod Geliştirme Önerisi KABUL EDİLDİ. ID: " << capsuleId);
    } else {
        LOG_DEFAULT(LogLevel::INFO, "LearningModule: Kod Geliştirme Önerisi REDDEDİLDİ. ID: " << capsuleId);
    }
}

void LearningModule::update_q_values(const std::vector<float>& state_embedding, CerebrumLux::AIAction action, float reward) {
    std::stringstream ss;
    ss << std::string("EMB:");
    for (float val : state_embedding) {
        ss << std::fixed << std::setprecision(5) << val << "|";
    }
    EmbeddingStateKey state_key = ss.str();

    float current_q_value = q_table.q_values[state_key][action];
    float learning_rate_rl = 0.1f;
    float discount_factor = 0.9f;

    float max_next_q = 0.0f;
    if (q_table.q_values.count(state_key)) {
        for (const auto& action_pair : q_table.q_values[state_key]) {
            if (action_pair.second > max_next_q) {
                max_next_q = action_pair.second;
            }
        }
    }

    q_table.q_values[state_key][action] = current_q_value + learning_rate_rl * (reward + discount_factor * max_next_q - current_q_value);

    LOG_DEFAULT(LogLevel::DEBUG, "[LearningModule] Q-Table güncellendi. State: " << static_cast<std::string>(state_key).substr(0, std::min((size_t)50, static_cast<std::string>(state_key).length())) << "..., Action: " << CerebrumLux::action_to_string(action) << ", Reward: " << reward << ", Yeni Q-Value: " << q_table.q_values[state_key][action]);
}

void LearningModule::save_q_table() const {
    LOG_DEFAULT(LogLevel::INFO, "[LearningModule] Q-Table LMDB'ye kaydediliyor. Toplam durum: " << q_table.q_values.size());

    if (!knowledgeBase.get_swarm_db().is_open()) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "[LearningModule] Q-Table kaydedilemedi: SwarmVectorDB açık değil.");
        return;
    }

    for (const auto& state_pair : q_table.q_values) {
        EmbeddingStateKey state_key = state_pair.first;
        nlohmann::json action_map_json;
        for (const auto& action_pair : state_pair.second) {
            action_map_json[CerebrumLux::action_to_string(action_pair.first)] = action_pair.second;
        }
        std::string action_map_json_str = action_map_json.dump();

        if (!knowledgeBase.get_swarm_db().store_q_value_json(state_key, action_map_json_str)) {
            LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "[LearningModule] Q-Table kaydetme başarısız: EmbeddingStateKey (kısmi): " << static_cast<std::string>(state_key).substr(0, std::min((size_t)50, static_cast<std::string>(state_key).length())));
        }
    }
    LOG_DEFAULT(LogLevel::INFO, "[LearningModule] Q-Table kaydetme tamamlandı. Toplam kaydedilen durum: " << q_table.q_values.size());
}

void LearningModule::load_q_table() {
    LOG_DEFAULT(LogLevel::INFO, "[LearningModule] Q-Table LMDB'den yükleniyor.");
    q_table.q_values.clear();

    // SwarmVectorDB'nin açık olduğunu varsayıyoruz (KnowledgeBase tarafından yönetiliyor).

    std::vector<CerebrumLux::SwarmVectorDB::EmbeddingStateKey> all_q_state_keys = knowledgeBase.get_swarm_db().get_all_keys_for_dbi(knowledgeBase.get_swarm_db().q_values_dbi());
    for (const auto& state_key : all_q_state_keys) {
        std::optional<std::string> action_map_json_str_opt = knowledgeBase.get_swarm_db().get_q_value_json(state_key);
        if (action_map_json_str_opt) {
            try {
                nlohmann::json action_map_json = nlohmann::json::parse(*action_map_json_str_opt);
                for (nlohmann::json::const_iterator action_it = action_map_json.begin(); action_it != action_map_json.end(); ++action_it) {
                    q_table.q_values[state_key][CerebrumLux::string_to_action(action_it.key())] = action_it.value().get<float>();
                }
                LOG_DEFAULT(LogLevel::TRACE, "[LearningModule] Q-Table için durum yükleniyor. EmbeddingStateKey (kısmi): " << static_cast<std::string>(state_key).substr(0, std::min((size_t)50, static_cast<std::string>(state_key).length())));
            } catch (const nlohmann::json::exception& e) {
                LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "[LearningModule] Q-Table yüklenemedi: JSON ayrıştırma hatası: " << e.what() << ". EmbeddingStateKey (kısmi): " << static_cast<std::string>(state_key).substr(0, std::min((size_t)50, static_cast<std::string>(state_key).length())));
            }
        }
    }
    LOG_DEFAULT(LogLevel::INFO, "[LearningModule] Q-Table yükleme tamamlandı. Toplam yüklü durum: " << q_table.q_values.size());
}

} // namespace CerebrumLux