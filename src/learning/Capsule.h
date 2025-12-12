#ifndef CAPSULE_H
#define CAPSULE_H

#include <string>
#include <vector>
#include <chrono>
#include <nlohmann/json.hpp> // JSON serileştirme/deserileştirme için
#include <iomanip> // std::put_time için
#include <sstream> // std::stringstream için

namespace CerebrumLux {

struct Capsule {
    std::string id;
    float trust_score = 1.0f;
    std::chrono::system_clock::time_point timestamp_utc;
    std::string topic;
    std::string source;
    std::string content; // Şifresi çözülmüş/orijinal içerik
    std::string plain_text_summary;
    std::string cryptofig_blob_base64;
    std::vector<float> embedding; // Cryptofig'in float vektör hali
    float confidence;

    // Güvenlik ve Şifreleme alanları
    std::string encrypted_content;      // Şifreli içerik (ciphertext_base64)
    std::string signature_base64;       // Ed25519 imzası Base64
    std::string encryption_iv_base64;   // AES-GCM IV'si Base64
    std::string gcm_tag_base64;         // YENİ: AES-GCM tag'ı Base64
    std::string code_file_path;         // Düzeltme: İlişkili kod dosyasının yolu (eğer bir kod önerisiyse)

    // NLOHMANN_DEFINE_TYPE_INTRUSIVE makrosu std::chrono::time_point'ı doğrudan desteklemez.
    // Manuel to_json ve from_json fonksiyonları tanımlamalıyız.

    // to_json için kural
    friend void to_json(nlohmann::json& j, const Capsule& c) {
        j["id"] = c.id;
        j["trust_score"] = c.trust_score;
        
        // timestamp_utc'yi ISO 8601 string'e dönüştür
        std::time_t t = std::chrono::system_clock::to_time_t(c.timestamp_utc);
        std::stringstream ss;
        ss << std::put_time(std::gmtime(&t), "%Y-%m-%dT%H:%M:%SZ"); // ISO 8601 formatı
        j["timestamp_utc"] = ss.str();

        j["topic"] = c.topic;
        j["source"] = c.source;
        j["content"] = c.content;
        j["plain_text_summary"] = c.plain_text_summary;
        j["cryptofig_blob_base64"] = c.cryptofig_blob_base64;
        j["embedding"] = c.embedding;
        j["confidence"] = c.confidence;
        j["encrypted_content"] = c.encrypted_content;
        j["signature_base64"] = c.signature_base64;
        j["encryption_iv_base64"] = c.encryption_iv_base64;
        j["gcm_tag_base64"] = c.gcm_tag_base64;
        j["code_file_path"] = c.code_file_path; //EKLENDİ: code_file_path JSON'a yazılıyor
    }

    // from_json için kural
    friend void from_json(const nlohmann::json& j, Capsule& c) {
        // Essential fields - throw error if missing
        j.at("id").get_to(c.id);
        j.at("content").get_to(c.content);
        
        // Optional fields - use .value() with defaults
        c.trust_score = j.value("trust_score", 1.0f);
        c.topic = j.value("topic", "DefaultTopic");
        c.source = j.value("source", "DefaultSource");
        c.plain_text_summary = j.value("plain_text_summary", "");
        c.cryptofig_blob_base64 = j.value("cryptofig_blob_base64", "");
        c.embedding = j.value("embedding", std::vector<float>{});
        c.confidence = j.value("confidence", 0.5f);
        c.encrypted_content = j.value("encrypted_content", "");
        c.signature_base64 = j.value("signature_base64", "");
        c.encryption_iv_base64 = j.value("encryption_iv_base64", "");
        c.gcm_tag_base64 = j.value("gcm_tag_base64", "");
        c.code_file_path = j.value("code_file_path", "");

        // Timestamp with fallback
        if (j.contains("timestamp_utc")) {
            std::string ts_str = j.at("timestamp_utc").get<std::string>();
            std::tm tm = {};
            std::stringstream ss(ts_str);
            ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
            if (ss.fail()) { // Parsing failed, default to current time
                c.timestamp_utc = std::chrono::system_clock::now();
            } else {
                c.timestamp_utc = std::chrono::system_clock::from_time_t(std::mktime(&tm));
            }
        } else {
            // "timestamp_utc" yoksa, mevcut zamanı varsayılan olarak ayarla
            c.timestamp_utc = std::chrono::system_clock::now();
        }
    }
};

} // namespace CerebrumLux

#endif // CAPSULE_H