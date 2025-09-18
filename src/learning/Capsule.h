#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

struct Capsule {
    int id;                          // YENİ: SimulationData ile uyumlu olması için ID eklendi
    std::string content;             // Orijinal içerik (plain text)
    std::string source;              // Kaynak (manual, web vs.)
    std::string topic;               // Konu başlığı
    std::string encrypted_content;   // Cryptofig ile şifrelenmiş içerik
    std::vector<float> embedding;    // Embedding vektörü
    float confidence;                // YENİ: Güven skoru eklendi

    // JSON’a serialize
    nlohmann::json toJson() const {
        return {
            {"id", id}, // YENİ
            {"content", content},
            {"source", source},
            {"topic", topic},
            {"encrypted_content", encrypted_content},
            {"embedding", embedding},
            {"confidence", confidence} // YENİ
        };
    }

    // JSON’dan deserialize
    static Capsule fromJson(const nlohmann::json& j) {
        Capsule c;
        c.id = j.value("id", 0); // YENİ
        c.content = j.value("content", "");
        c.source = j.value("source", "");
        c.topic = j.value("topic", "");
        c.encrypted_content = j.value("encrypted_content", "");
        c.embedding = j.value("embedding", std::vector<float>{});
        c.confidence = j.value("confidence", 0.0f); // YENİ
        return c;
    }
};