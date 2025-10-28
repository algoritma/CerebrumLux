#ifndef SWARM_VECTORDB_DATAMODELS_H
#define SWARM_VECTORDB_DATAMODELS_H

#include <vector>
#include <string>
#include <cstdint> // uint8_t için

// Eigen kütüphanesi için
#include <Eigen/Dense>

namespace CerebrumLux {
namespace SwarmVectorDB {

// Kriptofig-Vektör Hibriti (CryptofigVector)
// Tanım: Kriptofigler, 128D float16 vektör embedding’leriyle birleştirilir.
// Her biri veri (metin, görüntü, Q-tablosu) ve Fisher sorusu taşır.
struct CryptofigVector {
    std::vector<uint8_t> cryptofig; // Şifreli veri (~512 byte veya daha az)
    Eigen::VectorXf embedding;      // 128D float vektör (256 byte, float16 yerine float kullanıldı)
    std::string fisher_query;       // Fisher sorusu (örn., "Bu veri doğru mu?")
    std::string id;                 // Kapsül ID'sine karşılık gelen benzersiz ID
    std::string content_hash;       // İçerik hash'i, tekillik için

    // Kurucu
    CryptofigVector() : embedding(128) {} // 128D vektör başlat
    CryptofigVector(const std::vector<uint8_t>& crypto, const Eigen::VectorXf& emb, const std::string& query, const std::string& capsule_id, const std::string& hash)
        : cryptofig(crypto), embedding(emb), fisher_query(query), id(capsule_id), content_hash(hash) {}
};

} // namespace SwarmVectorDB
} // namespace CerebrumLux

#endif // SWARM_VECTORDB_DATAMODELS_H