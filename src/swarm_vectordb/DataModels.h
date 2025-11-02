#ifndef SWARM_VECTORDB_DATAMODELS_H
#define SWARM_VECTORDB_DATAMODELS_H

#include <vector>
#include <string>
#include <cstdint> // uint8_t için

#include "../external/nlohmann/json.hpp" // JSON serileştirme için
#include "../core/utils.h" // action_to_string ve string_to_action için
#include "../core/enums.h" // AIAction için

// Eigen kütüphanesi için
#include <Eigen/Dense>

namespace CerebrumLux {
namespace SwarmVectorDB {

struct CryptofigVector {
    std::vector<uint8_t> cryptofig; // Şifreli veri (~512 byte veya daha az)
    Eigen::VectorXf embedding;      // 128D float vektör (256 byte, float16 yerine float kullanıldı)
    std::string fisher_query;       // Fisher sorusu (örn., "Bu veri doğru mu?")
    std::string topic;              // YENİ: Kapsülün konu başlığı
    std::string id;                 // Kapsül ID'sine karşılık gelen benzersiz ID
    std::string content_hash;       // İçerik hash'i, tekillik için

    CryptofigVector() : embedding(128) {} // 128D vektör başlat
    CryptofigVector(const std::vector<uint8_t>& crypto, const Eigen::VectorXf& emb, const std::string& query, const std::string& capsule_topic, // topic yer değiştirildi
                    const std::string& capsule_id, const std::string& hash) // ID ve Hash sona alındı
        : cryptofig(crypto), embedding(emb), fisher_query(query), topic(capsule_topic), id(capsule_id), content_hash(hash) {}
};

// Sparse Q-Table (RL için)
// Durum anahtarı olarak embedding vektörlerinin string temsili kullanılabilir
using EmbeddingStateKey = std::string; 

struct SparseQTable {
    // Q-değerleri: EmbeddingStateKey (embedding string temsili) -> Action (enum) -> Q-Value (float)
    std::map<EmbeddingStateKey, std::map<CerebrumLux::AIAction, float>> q_values;

    SparseQTable() = default;
    // Kopyalama ve atama operatörleri varsayılan olarak kullanılabilir.

    // nlohmann::json ile serileştirme için friend fonksiyonlar
    friend void to_json(nlohmann::json& j, const SparseQTable& sqt) {
        nlohmann::json q_map_json;
        for (const auto& state_pair : sqt.q_values) {
            nlohmann::json action_map_json;
            for (const auto& action_pair : state_pair.second) {
                action_map_json[CerebrumLux::action_to_string(action_pair.first)] = action_pair.second;
            }
            q_map_json[state_pair.first] = action_map_json;
        }
        j["q_values"] = q_map_json;
    }

    friend void from_json(const nlohmann::json& j, SparseQTable& sqt) {
        sqt.q_values.clear();
        const nlohmann::json& q_map_json = j.at("q_values");
        for (nlohmann::json::const_iterator it = q_map_json.begin(); it != q_map_json.end(); ++it) {
            EmbeddingStateKey state_key = it.key();
            const nlohmann::json& action_map_json = it.value();
            for (nlohmann::json::const_iterator action_it = action_map_json.begin(); action_it != action_map_json.end(); ++action_it) {
                sqt.q_values[state_key][CerebrumLux::string_to_action(action_it.key())] = action_it.value().get<float>();
            }
        }
    }
};


} // namespace SwarmVectorDB
} // namespace CerebrumLux

#endif // SWARM_VECTORDB_DATAMODELS_H