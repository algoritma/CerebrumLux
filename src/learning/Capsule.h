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
    }

    // from_json için kural
    friend void from_json(const nlohmann::json& j, Capsule& c) {
        j.at("id").get_to(c.id);
        j.at("trust_score").get_to(c.trust_score);

        // ISO 8601 string'i timestamp_utc'ye dönüştür
        std::string ts_str = j.at("timestamp_utc").get<std::string>();
        std::tm tm = {};
        std::stringstream ss(ts_str);
        ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
        c.timestamp_utc = std::chrono::system_clock::from_time_t(std::mktime(&tm));
        
        j.at("topic").get_to(c.topic);
        j.at("source").get_to(c.source);
        j.at("content").get_to(c.content);
        j.at("plain_text_summary").get_to(c.plain_text_summary);
        j.at("cryptofig_blob_base64").get_to(c.cryptofig_blob_base64);
        j.at("embedding").get_to(c.embedding);
        j.at("confidence").get_to(c.confidence);
        j.at("encrypted_content").get_to(c.encrypted_content);
        j.at("signature_base64").get_to(c.signature_base64);
        j.at("encryption_iv_base64").get_to(c.encryption_iv_base64);
        j.at("gcm_tag_base64").get_to(c.gcm_tag_base64);
    }
};

} // namespace CerebrumLux

#endif // CAPSULE_H