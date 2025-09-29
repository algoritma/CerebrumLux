#include "KnowledgeBase.h"
#include "../core/logger.h" // LOG_DEFAULT için
#include "../core/enums.h" // LogLevel için
#include "../core/utils.h" // SafeRNG için
#include "../brain/autoencoder.h" // CryptofigAutoencoder::INPUT_DIM için
#include <fstream>
#include <algorithm> // std::remove_if için
#include <limits> // std::numeric_limits için
#include <chrono> // Time functions
#include <filesystem> // YENİ: std::filesystem için eklendi

namespace CerebrumLux {

// JSON serileştirme/deserileştirme için Capsule yapısının tanımlanması
// Bu kısım KnowledgeBase.cpp'den KALDIRILDI, çünkü Capsule.h içinde zaten tanımlıdır.
/*
void to_json(nlohmann::json& j, const Capsule& c) {
    j["id"] = c.id;
    j["content"] = c.content;
    j["source"] = c.source;
    j["topic"] = c.topic;
    j["confidence"] = c.confidence;
    j["plain_text_summary"] = c.plain_text_summary;
    j["timestamp_utc"] = std::chrono::duration_cast<std::chrono::milliseconds>(c.timestamp_utc.time_since_epoch()).count();
    j["embedding"] = c.embedding;
    j["cryptofig_blob_base64"] = c.cryptofig_blob_base64;
    j["encrypted_content"] = c.encrypted_content;
    j["gcm_tag_base64"] = c.gcm_tag_base64;
    j["encryption_iv_base64"] = c.encryption_iv_base64;
    j["signature_base64"] = c.signature_base64;
}

void from_json(const nlohmann::json& j, Capsule& c) {
    j.at("id").get_to(c.id);
    j.at("content").get_to(c.content);
    j.at("source").get_to(c.source);
    j.at("topic").get_to(c.topic);
    j.at("confidence").get_to(c.confidence);
    j.at("plain_text_summary").get_to(c.plain_text_summary);
    long long timestamp_ms = j.at("timestamp_utc").get<long long>();
    c.timestamp_utc = std::chrono::time_point<std::chrono::system_clock>(std::chrono::milliseconds(timestamp_ms));
    j.at("embedding").get_to(c.embedding);
    j.at("cryptofig_blob_base64").get_to(c.cryptofig_blob_base64);
    j.at("encrypted_content").get_to(c.encrypted_content);
    j.at("gcm_tag_base64").get_to(c.gcm_tag_base64);
    j.at("encryption_iv_base64").get_to(c.encryption_iv_base64);
    j.at("signature_base64").get_to(c.signature_base64);
}
*/

KnowledgeBase::KnowledgeBase() {
    LOG_DEFAULT(LogLevel::INFO, "KnowledgeBase: Initialized.");
}

void KnowledgeBase::add_capsule(const Capsule& capsule) {
    auto it = std::find_if(capsules.begin(), capsules.end(), [&](const Capsule& c){ return c.id == capsule.id; });
    if (it != capsules.end()) {
        *it = capsule;
        LOG_DEFAULT(LogLevel::DEBUG, "KnowledgeBase: Mevcut kapsül güncellendi. ID: " << capsule.id);
    } else {
        capsules.push_back(capsule);
        LOG_DEFAULT(LogLevel::INFO, "KnowledgeBase: New capsule added. ID: " << capsule.id);
    }
}

std::vector<Capsule> KnowledgeBase::semantic_search(const std::string& query, int top_k) const {
    LOG_DEFAULT(LogLevel::DEBUG, "KnowledgeBase: Semantic search for query: " << query << ", Top K: " << top_k);
    std::vector<Capsule> results;
    for (const auto& capsule : capsules) {
        if (capsule.topic.find(query) != std::string::npos || capsule.plain_text_summary.find(query) != std::string::npos) {
            results.push_back(capsule);
        }
    }
    if (results.empty()) {
        LOG_DEFAULT(LogLevel::WARNING, "KnowledgeBase: '" << query << "' sorgusu için sonuç bulunamadı.");
    }
    return results;
}

std::vector<Capsule> KnowledgeBase::search_by_topic(const std::string& topic) const {
    LOG_DEFAULT(LogLevel::DEBUG, "KnowledgeBase: Topic'e göre arama yapılıyor: " << topic);
    std::vector<Capsule> results;
    for (const auto& capsule : capsules) {
        if (capsule.topic == topic) {
            results.push_back(capsule);
        }
    }
    if (results.empty()) {
        LOG_DEFAULT(LogLevel::WARNING, "KnowledgeBase: '" << topic << "' konusu için sonuç bulunamadı.");
    }
    return results;
}

std::optional<Capsule> KnowledgeBase::find_capsule_by_id(const std::string& id) const {
    LOG_DEFAULT(LogLevel::DEBUG, "KnowledgeBase: ID'ye göre kapsül aranıyor: " << id);
    for (const auto& capsule : capsules) {
        if (capsule.id == id) {
            return capsule;
        }
    }
    LOG_DEFAULT(LogLevel::WARNING, "KnowledgeBase: ID '" << id << "' ile kapsül bulunamadı.");
    return std::nullopt;
}

void KnowledgeBase::quarantine_capsule(const std::string& id) {
    LOG_DEFAULT(LogLevel::INFO, "KnowledgeBase: Kapsül karantinaya alınıyor. ID: " << id);
    auto it = std::remove_if(capsules.begin(), capsules.end(), [&](const Capsule& c){ return c.id == id; });
    if (it != capsules.end()) {
        quarantined_capsules.push_back(*it);
        capsules.erase(it, capsules.end());
        LOG_DEFAULT(LogLevel::INFO, "KnowledgeBase: Kapsül karantinaya alındı. ID: " << id);
    } else {
        LOG_DEFAULT(LogLevel::WARNING, "KnowledgeBase: Karantinaya alınacak ID '" << id << "' ile kapsül bulunamadı.");
    }
}

void KnowledgeBase::revert_capsule(const std::string& id) {
    LOG_DEFAULT(LogLevel::INFO, "KnowledgeBase: Kapsül karantinadan geri alınıyor. ID: " << id);
    auto it = std::remove_if(quarantined_capsules.begin(), quarantined_capsules.end(), [&](const Capsule& c){ return c.id == id; });
    if (it != quarantined_capsules.end()) {
        capsules.push_back(*it);
        quarantined_capsules.erase(it, quarantined_capsules.end());
        LOG_DEFAULT(LogLevel::INFO, "KnowledgeBase: Kapsül karantinadan geri alındı. ID: " << id);
    } else {
        LOG_DEFAULT(LogLevel::WARNING, "KnowledgeBase: Karantinadan geri alınacak ID '" << id << "' ile kapsül bulunamadı.");
    }
}

void KnowledgeBase::save(const std::string& filename) const {
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "KnowledgeBase: Bilgi tabanı kaydediliyor: " << filename);
    nlohmann::json j;
    j["active_capsules"] = capsules;
    j["quarantined_capsules"] = quarantined_capsules;

    // YENİ: Tam dosya yolunu logla
    std::filesystem::path current_path = std::filesystem::current_path();
    std::filesystem::path file_path = current_path / filename;
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "KnowledgeBase: Kaydedilmeye calisilan tam dosya yolu: " << file_path.string());

    std::ofstream o(file_path); // file_path kullan
    if (o.is_open()) {
        o << std::setw(4) << j << std::endl;
        LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "KnowledgeBase: Bilgi tabanı başarıyla kaydedildi.");
    } else {
        LOG_ERROR_CERR(CerebrumLux::LogLevel::ERR_CRITICAL, "KnowledgeBase: Bilgi tabanı dosyaya kaydedilemedi: " << file_path.string());
    }
}

void KnowledgeBase::load(const std::string& filename) {
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "KnowledgeBase: Bilgi tabanı yükleniyor: " << filename);

    // YENİ DİYAGNOSTİK KOD:
    std::filesystem::path current_path = std::filesystem::current_path();
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "KnowledgeBase: Uygulama calisma dizini: " << current_path.string());
    std::filesystem::path file_path = current_path / filename;
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "KnowledgeBase: Yuklenmeye calisilan tam dosya yolu: " << file_path.string());

    if (!std::filesystem::exists(file_path)) {
        LOG_DEFAULT(CerebrumLux::LogLevel::WARNING, "KnowledgeBase: Dosya bulunamadi: " << file_path.string() << ". Bos bilgi tabani ile baslatiliyor.");
        capsules.clear();
        quarantined_capsules.clear();
        return; // Dosya yoksa daha fazla deneme
    } else {
        LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "KnowledgeBase: Dosya var: " << file_path.string());
    }
    // END YENİ DİYAGNOSTİK KOD

    std::ifstream i(file_path); // YENİ: Tam dosya yolunu kullanın
    if (i.is_open()) {
        try {
            nlohmann::json j;
            i >> j;
            if (j.count("active_capsules")) {
                capsules = j.at("active_capsules").get<std::vector<Capsule>>();
            }
            if (j.count("quarantined_capsules")) {
                quarantined_capsules = j.at("quarantined_capsules").get<std::vector<Capsule>>();
            }
            LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "KnowledgeBase: Bilgi tabanı başarıyla yüklendi. Aktif kapsül sayısı: " << capsules.size() << ", Karantinaya alınan kapsül sayısı: " << quarantined_capsules.size());
        } catch (const nlohmann::json::exception& e) {
            LOG_ERROR_CERR(CerebrumLux::LogLevel::ERR_CRITICAL, "KnowledgeBase: JSON yükleme hatası: " << e.what() << ". Boş bilgi tabanı ile başlatılıyor.");
            capsules.clear();
            quarantined_capsules.clear();
        } catch (const std::exception& e) {
            LOG_ERROR_CERR(CerebrumLux::LogLevel::ERR_CRITICAL, "KnowledgeBase: Bilgi tabanı yükleme sırasında genel hata: " << e.what() << ". Boş bilgi tabanı ile başlatılıyor.");
            capsules.clear();
            quarantined_capsules.clear();
        }
    } else {
        // Bu blok artık sadece 'std::filesystem::exists' true döndürmesine rağmen 'is_open()' başarısız olursa çalışır.
        // Bu durumda ya izin sorunudur ya da dosya kilitlidir.
        LOG_DEFAULT(CerebrumLux::LogLevel::WARNING, "KnowledgeBase: Dosya acilamadi: " << file_path.string() << ". Dosya var ancak acilamiyor (izin veya kilit sorunu olabilir). Bos bilgi tabani ile baslatiliyor.");
        capsules.clear();
        quarantined_capsules.clear();
    }
}

std::vector<float> KnowledgeBase::computeEmbedding(const std::string& text) const {
    LOG_DEFAULT(LogLevel::DEBUG, "KnowledgeBase: computeEmbedding (LearningModule tarafından çağrılmalı).");
    return std::vector<float>(CryptofigAutoencoder::INPUT_DIM, 0.0f);
}

float KnowledgeBase::cosineSimilarity(const std::vector<float>& vec1, const std::vector<float>& vec2) const {
    LOG_DEFAULT(LogLevel::DEBUG, "KnowledgeBase: cosineSimilarity (henüz implemente edilmedi).");
    if (vec1.empty() || vec2.empty() || vec1.size() != vec2.size()) {
        return 0.0f;
    }
    return SafeRNG::get_instance().get_float(0.0f, 1.0f);
}

std::vector<Capsule> KnowledgeBase::get_all_capsules() const {
    LOG_DEFAULT(LogLevel::DEBUG, "KnowledgeBase: Tüm aktif kapsüller isteniyor. Sayı: " << capsules.size());
    return capsules;
}

} // namespace CerebrumLux