#ifndef SWARM_VECTORDB_VECTORDB_H
#define SWARM_VECTORDB_VECTORDB_H

#include <string>
#include <vector>
#include <memory> // std::unique_ptr için
#include <mutex> // LMDB erişimi için mutex
#include <map> // hnswlib label'larını ID'lerle eşlemek için
#include <nlohmann/json.hpp> // JSON serileştirme için

#include "../core/logger.h" // CerebrumLux Logger için
#include "DataModels.h"     // CryptofigVector için
#include "../hnswlib_wrapper.h" // HNSWIndex wrapper için
#include "../learning/StrategyOutcome.h" // StrategyOutcome için
#include "../core/enums.h" // UserIntent için
#include "../ai_tutor/enums.h" // TeachingStyle ve to_string fonksiyonları için

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
    std::unique_ptr<CryptofigVector> get_vector(const std::string& id, MDB_txn* existing_txn = nullptr) const; // Keep consistent
    // Verilen hash'e sahip vektörü siler

    // YENİ: Toplu okuma fonksiyonu (Performans optimizasyonu)
    std::vector<std::unique_ptr<CryptofigVector>> get_vectors_batch(const std::vector<std::string>& ids) const;

    bool delete_vector(const std::string& id);
    // Veritabanının açık olup olmadığını döndürür
    bool is_open() const { return env_ != nullptr; }
    // En yakın N vektörü arar (hnswlib kullanır)
    std::vector<std::string> search_similar_vectors(const std::vector<float>& query_embedding, int top_k) const;
    // Veritabanındaki tüm ID'leri döndürür (dikkat: büyük DB'lerde yavaş olabilir)
    std::vector<std::string> get_all_ids() const;

    // SparseQTable kalıcılığı için yeni genel metotlar
    bool store_q_value_json(const EmbeddingStateKey& state_key, const std::string& action_map_json_str);
    std::optional<std::string> get_q_value_json(const EmbeddingStateKey& state_key) const;
    bool delete_q_value_json(const EmbeddingStateKey& state_key);

    // YENİ EKLENDİ: Kapsül içeriklerini yönetmek için metotlar
    bool store_capsule_content(const std::string& id, const std::string& content, MDB_txn* existing_txn = nullptr);
    std::optional<std::string> get_capsule_content(const std::string& id, MDB_txn* existing_txn = nullptr) const;

    // Belirli bir DBI'daki tüm anahtarları döndürür.
    std::vector<EmbeddingStateKey> get_all_keys_for_dbi(MDB_dbi dbi) const;

    // YENİ: Öğretme stratejisi sonuçlarını kaydetmek ve yüklemek için metotlar
    bool store_strategy_outcome(UserIntent intent, const StrategyOutcome& outcome);
    std::vector<StrategyOutcome> load_strategy_history(UserIntent intent, int limit) const;


    // LMDB ortam ve DBI handle'ları için getter'lar (list_data için gerekli)
    MDB_env* get_env() const { return env_; }
    MDB_dbi get_dbi() const { return dbi_; }
    MDB_dbi q_values_dbi() const { return q_values_dbi_; } // YENİ: q_values_dbi_ için getter


private:
    std::string db_path_;
    MDB_env* env_; // LMDB ortamı
    MDB_dbi dbi_;                         // LMDB veritabanı handle'ı
    mutable std::mutex mutex_;            // Thread güvenliği için (const metotlarda kilitlenebilmesi için mutable eklendi)
    MDB_dbi hnsw_label_to_id_map_dbi_;    // HNSW label -> CryptofigVector ID haritası için DBI
    MDB_dbi id_to_hnsw_label_map_dbi_;    // CryptofigVector ID -> HNSW label haritası için DBI
    MDB_dbi hnsw_next_label_dbi_;         // Bir sonraki hnswlib label değerini saklamak için DBI
    MDB_dbi q_values_dbi_;                // SparseQTable'ın Q-değerlerini (EmbeddingStateKey -> ActionMap JSON) saklamak için DBI

    MDB_dbi q_metadata_dbi_;              // SparseQTable ile ilgili meta verileri (örn. öğrenme oranları) JSON olarak saklamak için DBI

    // YENİ: Öğretme stratejisi sonuçları için DBI
    MDB_dbi strategy_outcome_dbi_;

    // HNSW index'i std::unique_ptr ile yönetiyoruz
    std::unique_ptr<CerebrumLux::HNSW::HNSWIndex> hnsw_index_; 
    hnswlib::labeltype next_hnsw_label_ = 0; // HNSW index'e eklenecek bir sonraki etiket
    std::map<hnswlib::labeltype, std::string> hnsw_label_to_id_map_; // HNSW etiketlerini CryptofigVector ID'lerine eşler
    std::map<std::string, hnswlib::labeltype> id_to_hnsw_label_map_; // CryptofigVector ID'lerini HNSW etiketlerine eşler

    MDB_dbi capsule_content_dbi_; // Kapsül içeriklerini saklamak için
    std::vector<std::string> get_all_ids_internal(MDB_txn* txn) const; // Yeni internal metot

    void close_internal(); // Mutex kilidi olmadan kapatma mantığını yönetir
   
    // Kopyalama ve atamayı engelle
    SwarmVectorDB(const SwarmVectorDB&) = delete;
    SwarmVectorDB& operator=(const SwarmVectorDB&) = delete;
};

// EmbeddingStateKey'i SwarmVectorDB namespace'i içinde tanımla
using EmbeddingStateKey = std::string;

} // namespace SwarmVectorDB
} // namespace CerebrumLux

#endif // SWARM_VECTORDB_VECTORDB_H