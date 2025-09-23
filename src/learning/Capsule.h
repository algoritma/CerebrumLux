#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <chrono> // For std::chrono::system_clock::time_point

struct Capsule {
    std::string id;                          // YENİ: ID artık string
    float trust_score;                       // YENİ: Güven skoru
    std::chrono::system_clock::time_point timestamp_utc; // YENİ: UTC zaman damgası
    std::vector<std::string> keywords;       // YENİ: Anahtar kelimeler
    float estimated_cost;                    // YENİ: Tahmini maliyet
    std::string plain_text_summary;          // YENİ: Düz metin özeti
    std::string cryptofig_blob_base64;       // YENİ: Kriptofig blob'unun Base64 kodu
    std::string signature_base64;            // YENİ: İmzanın Base64 kodu
    std::string encryption_iv_base64;        // YENİ: Şifreleme IV'sinin Base64 kodu

    // Mevcut alanlar (bazılarının tipi değişti)
    std::string content;             // Orijinal içerik (plain text)
    std::string source;              // Kaynak (manual, web vs.)
    std::string topic;               // Konu başlığı
    std::string encrypted_content;   // Cryptofig ile şifrelenmiş içerik (artık raw, base64 blob'u ile farklılaşacak)
    std::vector<float> embedding;    // Embedding vektörü
    float confidence;                // Güven skoru

    // JSON’a serialize
    nlohmann::json toJson() const {
        // time_point'u ISO 8601 formatında string'e dönüştürme
        std::time_t tt = std::chrono::system_clock::to_time_t(timestamp_utc);
        std::tm tm = *std::gmtime(&tt); // UTC için gmtime
        std::stringstream ss;
        ss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
        std::string timestamp_str = ss.str();

        return {
            {"id", id},
            {"trust_score", trust_score},
            {"timestamp_utc", timestamp_str},
            {"keywords", keywords},
            {"estimated_cost", estimated_cost},
            {"plain_text_summary", plain_text_summary},
            {"cryptofig_blob_base64", cryptofig_blob_base64},
            {"signature_base64", signature_base64},
            {"encryption_iv_base64", encryption_iv_base64},
            {"content", content},
            {"source", source},
            {"topic", topic},
            {"encrypted_content", encrypted_content},
            {"embedding", embedding},
            {"confidence", confidence}
        };
    }

    // JSON’dan deserialize
    static Capsule fromJson(const nlohmann::json& j) {
        Capsule c;
        c.id = j.value("id", "");
        c.trust_score = j.value("trust_score", 0.0f);
        
        // ISO 8601 formatından time_point'a dönüştürme
        std::string timestamp_str = j.value("timestamp_utc", "");
        if (!timestamp_str.empty()) {
            std::tm tm = {};
            std::stringstream ss(timestamp_str);
            ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
            if (!ss.fail()) {
                c.timestamp_utc = std::chrono::system_clock::from_time_t(std::mktime(&tm));
            }
        }

        c.keywords = j.value("keywords", std::vector<std::string>{});
        c.estimated_cost = j.value("estimated_cost", 0.0f);
        c.plain_text_summary = j.value("plain_text_summary", "");
        c.cryptofig_blob_base64 = j.value("cryptofig_blob_base64", "");
        c.signature_base64 = j.value("signature_base64", "");
        c.encryption_iv_base64 = j.value("encryption_iv_base64", "");

        c.content = j.value("content", "");
        c.source = j.value("source", "");
        c.topic = j.value("topic", "");
        c.encrypted_content = j.value("encrypted_content", "");
        c.embedding = j.value("embedding", std::vector<float>{});
        c.confidence = j.value("confidence", 0.0f);
        return c;
    }
};