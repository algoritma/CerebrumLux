#ifndef SWARM_VECTORDB_VECTORDB_H
#define SWARM_VECTORDB_VECTORDB_H

#include <string>
#include <vector>
#include <memory> // std::unique_ptr için
#include <mutex> // LMDB erişimi için mutex

#include "../core/logger.h" // CerebrumLux Logger için
#include "DataModels.h"     // CryptofigVector için

// OpenSSL SHA-256 için
#include <openssl/sha.h>

// LMDB için
#include <lmdb.h>

namespace CerebrumLux {
namespace SwarmVectorDB {

// Sürü Konsensüs Ağacı (Merkle Tree benzeri)
class SwarmConsensusTree {
public:
    SwarmConsensusTree();
    // CryptofigVector'den hash oluşturur ve ağacı günceller
    void update_tree(const CryptofigVector& cv);
    // Kök hash'i döndürür
    std::string get_root_hash() const { return root_hash_; }

private:
    std::vector<std::string> leaf_hashes_; // CryptofigVector hash'leri
    std::string root_hash_;
    std::mutex mutex_; // Thread güvenliği için
};

// Yerel Vektör Deposu (LMDB tabanlı)
class SwarmVectorDB {
public:
    SwarmVectorDB(const std::string& db_path);
    ~SwarmVectorDB();

    // Vektör veritabanını açar veya oluşturur
    bool open();
    // Vektör veritabanını kapatır
    void close();

    // CryptofigVector'ü veritabanına depolar
    bool store_vector(const CryptofigVector& cv);
    // Verilen hash'e sahip CryptofigVector'ü veritabanından getirir
    std::unique_ptr<CryptofigVector> get_vector(const std::string& hash);
    // Verilen hash'e sahip vektörü siler
    bool delete_vector(const std::string& hash);

private:
    std::string db_path_;
    MDB_env* env_ = nullptr; // LMDB ortamı
    MDB_dbi dbi_ = 0;        // LMDB veritabanı handle'ı
    std::mutex mutex_;       // Thread güvenliği için

    // Kopyalama ve atamayı engelle
    SwarmVectorDB(const SwarmVectorDB&) = delete;
    SwarmVectorDB& operator=(const SwarmVectorDB&) = delete;
};

} // namespace SwarmVectorDB
} // namespace CerebrumLux

#endif // SWARM_VECTORDB_VECTORDB_H