#ifndef SWARM_VECTORDB_DATAMODELS_H
#define SWARM_VECTORDB_DATAMODELS_H

#include <vector>
#include <string>
#include <cstdint> // uint8_t için

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
using StateKey = std::string; 

struct SparseQTable {
    // Q-değerleri: Durum (string key) -> Eylem -> Değer
    std::map<StateKey, std::map<AIAction, float>> q_values;

    SparseQTable() = default;
    // Kopyalama ve atama operatörleri varsayılan olarak kullanılabilir.
};


} // namespace SwarmVectorDB
} // namespace CerebrumLux

#endif // SWARM_VECTORDB_DATAMODELS_H