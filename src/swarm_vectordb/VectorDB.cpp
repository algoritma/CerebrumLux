#include "VectorDB.h"
#include <iomanip> // std::hex, std::setw için
#include <sstream> // stringstream için
#include <stdexcept> // runtime_error için
#include <filesystem> // std::filesystem::create_directories için
#include <algorithm> // std::reverse için
#include <queue> // std::priority_queue için
#include <functional> // std::hash için

#include "DataModels.h" // CryptofigVector
#include "../core/logger.h" // LOG_DEFAULT için
#include "../hnswlib_wrapper.h" // HNSWIndex wrapper için

namespace fs = std::filesystem; // std::filesystem için alias

namespace CerebrumLux {
namespace SwarmVectorDB {

// --- SwarmConsensusTree Implementasyonu ---

SwarmConsensusTree::SwarmConsensusTree() {
    root_hash_ = "";
    LOG_DEFAULT(LogLevel::INFO, "SwarmConsensusTree: Başlatıldı.");
}

void SwarmConsensusTree::update_tree(const CryptofigVector& cv) {
    std::lock_guard<std::mutex> lock(mutex_); // Thread güvenliği
    
    // CryptofigVector'den hash oluştur (basit örnek)
    // Gerçek bir hash için CryptofigVector'ün tüm içeriği (cryptofig, embedding, fisher_query, id, content_hash)
    // byte dizisine dönüştürülüp hash'lenmelidir.
    std::string data_to_hash = cv.id + cv.content_hash + cv.fisher_query;
    for (uint8_t byte : cv.cryptofig) {
        data_to_hash += static_cast<char>(byte);
    }
    for (int i = 0; i < cv.embedding.size(); ++i) {
        data_to_hash += std::to_string(cv.embedding(i));
    }

    unsigned char hash_buf[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(data_to_hash.data()), data_to_hash.size(), hash_buf);

    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash_buf[i];
    }
    leaf_hashes_.push_back(ss.str());

    // Basit root hash güncellemesi: Tüm yaprak hash'lerini birleştirip tekrar hash'le
    // Gerçek Merkle Tree implementasyonu daha karmaşıktır.
    std::string combined_hashes;
    for (const auto& h : leaf_hashes_) {
        combined_hashes += h;
    }
    SHA256(reinterpret_cast<const unsigned char*>(combined_hashes.data()), combined_hashes.size(), hash_buf);
    ss.str(""); // Stringstream'i temizle
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash_buf[i];
    }
    root_hash_ = ss.str();

    LOG_DEFAULT(LogLevel::TRACE, "SwarmConsensusTree: Ağaç güncellendi. Kök Hash: " << root_hash_);
}


// --- SwarmVectorDB Implementasyonu (LMDB tabanlı) ---

SwarmVectorDB::SwarmVectorDB(const std::string& db_path)
    : db_path_(db_path), 
    hnsw_index_(std::make_unique<CerebrumLux::HNSW::HNSWIndex>(128, 100000)), // 128D embedding, max 100k eleman
    next_hnsw_label_(0) {
    hnsw_label_to_id_map_dbi_ = 0;
    id_to_hnsw_label_map_dbi_ = 0;
    hnsw_next_label_dbi_ = 0;
    LOG_DEFAULT(LogLevel::INFO, "SwarmVectorDB Kurucusu: Başlatıldı. DB Yolu: " << db_path_);
    env_ = nullptr; // env_ ve dbi_ üyelerini açıkça başlat
    dbi_ = 0;
}

SwarmVectorDB::~SwarmVectorDB() {
    close();
    LOG_DEFAULT(LogLevel::INFO, "SwarmVectorDB Yıkıcısı: Çağrıldı.");
}

bool SwarmVectorDB::open() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (env_ != nullptr) {
        LOG_DEFAULT(LogLevel::WARNING, "SwarmVectorDB::open(): LMDB ortamı zaten açık. Yeniden açmaya gerek yok.");
        return true;
    }

    LOG_DEFAULT(LogLevel::INFO, "SwarmVectorDB::open(): LMDB ortamı açma işlemi başlatılıyor...");

    int rc;

    // 1. Veritabanı dizininin var olduğundan emin ol
    try {
        if (!fs::exists(db_path_)) {
            fs::create_directories(db_path_);
            LOG_DEFAULT(LogLevel::TRACE, "SwarmVectorDB::open(): DB dizini oluşturuldu: " << db_path_);
        } else {
            LOG_DEFAULT(LogLevel::TRACE, "SwarmVectorDB::open(): DB dizini zaten mevcut: " << db_path_);
        }
    } catch (const fs::filesystem_error& e) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::open(): DB dizini oluşturma/kontrol hatası: " << e.what() << ", Yol: " << db_path_);
        return false;
    }

    // LMDB ortamı oluştur
    rc = mdb_env_create(&env_);
    if (rc != MDB_SUCCESS) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB: mdb_env_create başarısız: " << mdb_strerror(rc));
        return false;
    }

    LOG_DEFAULT(LogLevel::TRACE, "SwarmVectorDB::open(): mdb_env_create başarılı.");

    // 2. Ortam boyutunu ayarla (örn: 1GB)
    rc = mdb_env_set_mapsize(env_, 1ULL * 1024ULL * 1024ULL * 1024ULL); // 1 GB (1GB)
    if (rc != MDB_SUCCESS) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::open(): mdb_env_set_mapsize başarısız: " << mdb_strerror(rc));
        mdb_env_close(env_); env_ = nullptr;
        return false;
    }
    LOG_DEFAULT(LogLevel::TRACE, "SwarmVectorDB::open(): mdb_env_set_mapsize başarılı (1GB).");

    // 3. Maksimum veritabanı sayısını ayarla (CryptofigVector'lar için birincil DB ve potansiyel diğerleri)
    rc = mdb_env_set_maxdbs(env_, 10); // Maksimum 10 DBI
    if (rc != MDB_SUCCESS) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::open(): mdb_env_set_maxdbs başarısız: " << mdb_strerror(rc));
        mdb_env_close(env_); env_ = nullptr;
        return false;
    }
    LOG_DEFAULT(LogLevel::TRACE, "SwarmVectorDB::open(): mdb_env_set_maxdbs başarılı (10 DB).");

    // MDB_NOSUBDIR, dizini oluşturmayıp dosyaları doğrudan db_path'e yazar. Bu, bizim fs::create_directories ile çelişmez.
    // Ancak, eğer db_path zaten bir dosya ise MDB_NOSUBDIR sorun çıkarabilir.
    // Path bir dizin olduğundan, MDB_NOSUBDIR kullanımı sorunsuz olmalı.

    // 4. Ortamı aç. db_path_ bir dizin ve LMDB dosyalarını bu dizin içinde oluşturacaktır. MDB_NOSUBDIR kullanılmıyor. İzin modu (0) Windows'ta varsayılanı kullanır.
    rc = mdb_env_open(env_, db_path_.c_str(), 0, 0);
    if (rc != MDB_SUCCESS) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::open(): mdb_env_open başarısız: " << mdb_strerror(rc) << ", Yol: " << db_path_); // 'native_db_path' -> 'db_path_'
        mdb_env_close(env_); env_ = nullptr;
        return false;
    }
    LOG_DEFAULT(LogLevel::INFO, "SwarmVectorDB::open(): LMDB ortamı başarıyla açıldı. Yol: " << db_path_);

    // 5. Veritabanını aç (veya oluştur)
    MDB_txn* txn;
    rc = mdb_txn_begin(env_, nullptr, 0, &txn);
    if (rc != MDB_SUCCESS) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::open(): mdb_txn_begin başarısız: " << mdb_strerror(rc));
        mdb_env_close(env_); env_ = nullptr;
        return false;
    }
    LOG_DEFAULT(LogLevel::TRACE, "SwarmVectorDB::open(): mdb_txn_begin başarılı.");

    // Birincil veritabanını aç (CryptofigVector'lar için)
    rc = mdb_dbi_open(txn, "cryptofig_vectors", MDB_CREATE, &dbi_);
    if (rc != MDB_SUCCESS) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::open(): mdb_dbi_open 'cryptofig_vectors' başarısız: " << mdb_strerror(rc) << ", Yol: " << db_path_);
        mdb_txn_abort(txn);
        mdb_env_close(env_); env_ = nullptr;
        return false;
    }
    LOG_DEFAULT(LogLevel::TRACE, "SwarmVectorDB::open(): mdb_dbi_open 'cryptofig_vectors' başarılı.");

    // HNSW map'leri için DBI'ları aç
    rc = mdb_dbi_open(txn, "hnsw_label_to_id_map", MDB_CREATE, &hnsw_label_to_id_map_dbi_);
    if (rc != MDB_SUCCESS) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::open(): mdb_dbi_open 'hnsw_label_to_id_map' başarısız: " << mdb_strerror(rc));
        mdb_txn_abort(txn); mdb_env_close(env_); env_ = nullptr;
        return false;
    }
    LOG_DEFAULT(LogLevel::TRACE, "SwarmVectorDB::open(): mdb_dbi_open 'hnsw_label_to_id_map' başarılı.");

    rc = mdb_dbi_open(txn, "id_to_hnsw_label_map", MDB_CREATE, &id_to_hnsw_label_map_dbi_);
    if (rc != MDB_SUCCESS) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::open(): mdb_dbi_open 'id_to_hnsw_label_map' başarısız: " << mdb_strerror(rc));
        mdb_txn_abort(txn); mdb_env_close(env_); env_ = nullptr;
        return false;
    }
    LOG_DEFAULT(LogLevel::TRACE, "SwarmVectorDB::open(): mdb_dbi_open 'id_to_hnsw_label_map' başarılı.");

    rc = mdb_dbi_open(txn, "hnsw_next_label_dbi", MDB_CREATE, &hnsw_next_label_dbi_);
    if (rc != MDB_SUCCESS) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::open(): mdb_dbi_open 'hnsw_next_label_dbi' başarısız: " << mdb_strerror(rc));
        mdb_txn_abort(txn); mdb_env_close(env_); env_ = nullptr;
        return false;
    }
    LOG_DEFAULT(LogLevel::TRACE, "SwarmVectorDB::open(): mdb_dbi_open 'hnsw_next_label_dbi' başarılı.");

    // YENİ: Q-Table DBI'larını aç
    rc = mdb_dbi_open(txn, "q_values_db", MDB_CREATE, &q_values_dbi_);
    if (rc != MDB_SUCCESS) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::open(): mdb_dbi_open 'q_values_db' başarısız: " << mdb_strerror(rc));
        mdb_txn_abort(txn); mdb_env_close(env_); env_ = nullptr;
        return false;
    }
    LOG_DEFAULT(LogLevel::TRACE, "SwarmVectorDB::open(): mdb_dbi_open 'q_values_db' başarılı.");

    rc = mdb_dbi_open(txn, "q_metadata_db", MDB_CREATE, &q_metadata_dbi_);
    if (rc != MDB_SUCCESS) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::open(): mdb_dbi_open 'q_metadata_db' başarısız: " << mdb_strerror(rc));
        mdb_txn_abort(txn); mdb_env_close(env_); env_ = nullptr;
        return false;
    }
    LOG_DEFAULT(LogLevel::TRACE, "SwarmVectorDB::open(): mdb_dbi_open 'q_metadata_db' başarılı.");

    rc = mdb_txn_commit(txn);
    if (rc != MDB_SUCCESS) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::open(): mdb_txn_commit başarısız: " << mdb_strerror(rc) << ", Yol: " << db_path_);
        mdb_env_close(env_); env_ = nullptr;
        return false;
    }
    LOG_DEFAULT(LogLevel::INFO, "SwarmVectorDB::open(): LMDB ortamı ve tüm DB'ler başarıyla açıldı.");
    
    // next_hnsw_label_ yükle (yeni bir read transaction içinde)
    MDB_txn* read_label_txn = nullptr; // Initialize to nullptr
    rc = mdb_txn_begin(env_, nullptr, MDB_RDONLY, &read_label_txn);
    if (rc != MDB_SUCCESS) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::open(): mdb_txn_begin başarısız (next_hnsw_label okuma): " << mdb_strerror(rc));
        // Kritik bir hata durumunda bile uygulamanın devam etmesi için next_hnsw_label_ 0 olarak ayarlanır
        next_hnsw_label_ = 0;
    } else {
        MDB_val key, data;
        std::string next_label_key_str = "next_hnsw_label"; // Fixed key for next_hnsw_label_
        key.mv_size = next_label_key_str.size();
        key.mv_data = (void*)next_label_key_str.data(); // Pointer to string data
        rc = mdb_get(read_label_txn, hnsw_next_label_dbi_, &key, &data);
        if (rc == MDB_SUCCESS && data.mv_size > 0) { // Check data.mv_size as well
            next_hnsw_label_ = static_cast<hnswlib::labeltype>(std::stoul(std::string(static_cast<char*>(data.mv_data), data.mv_size)));
            LOG_DEFAULT(LogLevel::INFO, "SwarmVectorDB::open(): Kaydedilmis next_hnsw_label: " << next_hnsw_label_);
        } else {
            LOG_DEFAULT(LogLevel::INFO, "SwarmVectorDB::open(): next_hnsw_label bulunamadi veya ilk kez baslatiliyor (Hata kodu: " << rc << ", Boyut: " << data.mv_size << ").");
            next_hnsw_label_ = 0; // Başlangıç değeri
        }
        mdb_txn_abort(read_label_txn); // Salt okunur işlem, iptal edilebilir.
    }

    // HNSW index'i yükle veya oluştur
    if (hnsw_index_) { // unique_ptr null değilse
        if (!hnsw_index_->load_or_create_index(db_path_ + "/hnsw_index.bin")) {
            LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::open(): HNSW index'i yüklenemedi veya olusturulamadi.");
            // Hata durumunda ne yapılacağına karar ver (LMDB açık kalır ama HNSW işlevsel olmaz)
            // return false; // HNSW hatası kritikse buradan dönebiliriz. Şimdilik devam ediyoruz.
        }

        // Eğer index yüklendi ve doluysa, hnsw_label_to_id_map ve id_to_hnsw_label_map'i LMDB'den yeniden inşa et
        if (hnsw_index_->get_current_elements() > 0) {
            LOG_DEFAULT(LogLevel::INFO, "SwarmVectorDB::open(): HNSW index zaten dolu, ID haritalari LMDB'den okunuyor.");
            
            MDB_txn* read_map_txn;
            rc = mdb_txn_begin(env_, nullptr, MDB_RDONLY, &read_map_txn);
            if (rc != MDB_SUCCESS) {
                LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::open(): mdb_txn_begin başarısız (harita okuma): " << mdb_strerror(rc) << ").");
                return false;
            }

            MDB_cursor* cursor_label_to_id;
            rc = mdb_cursor_open(read_map_txn, hnsw_label_to_id_map_dbi_, &cursor_label_to_id);
            if (rc != MDB_SUCCESS) { // If it fails, log and continue. We don't abort the entire open.
                LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::open(): mdb_cursor_open başarısız (label to id): " << mdb_strerror(rc));
                mdb_cursor_close(cursor_label_to_id); // Ensure cursor is closed on error
                mdb_txn_abort(read_map_txn);
                return false;
            }

            MDB_val map_key, map_data;
            while ((rc = mdb_cursor_get(cursor_label_to_id, &map_key, &map_data, MDB_NEXT)) == MDB_SUCCESS && map_key.mv_size > 0 && map_data.mv_size > 0) {
                hnswlib::labeltype label = static_cast<hnswlib::labeltype>(std::stoul(std::string(static_cast<char*>(map_key.mv_data), map_key.mv_size)));
                std::string id_str(static_cast<char*>(map_data.mv_data), map_data.mv_size);
                hnsw_label_to_id_map_[label] = id_str;
                if (label >= next_hnsw_label_) { // Update next_hnsw_label_ based on loaded labels
                    next_hnsw_label_ = label + 1;
                }
            }
            mdb_cursor_close(cursor_label_to_id);

            MDB_cursor* cursor_id_to_label;
            rc = mdb_cursor_open(read_map_txn, id_to_hnsw_label_map_dbi_, &cursor_id_to_label);
            if (rc != MDB_SUCCESS) { // If it fails, log and continue. We don't abort the entire open.
                LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::open(): mdb_cursor_open başarısız (id to label): " << mdb_strerror(rc));
                mdb_cursor_close(cursor_id_to_label); // Ensure cursor is closed on error
                mdb_txn_abort(read_map_txn);
                return false;
            }

            while ((rc = mdb_cursor_get(cursor_id_to_label, &map_key, &map_data, MDB_NEXT)) == MDB_SUCCESS && map_key.mv_size > 0 && map_data.mv_size > 0) {
                std::string id_str(static_cast<char*>(map_key.mv_data), map_key.mv_size);
                hnswlib::labeltype label = static_cast<hnswlib::labeltype>(std::stoul(std::string(static_cast<char*>(map_data.mv_data), map_data.mv_size)));
            }
            mdb_cursor_close(cursor_id_to_label); // Close cursor
            mdb_txn_abort(read_map_txn); // Abort read-only transaction (can be committed or aborted)

            LOG_DEFAULT(LogLevel::INFO, "SwarmVectorDB::open(): LMDB'den " << hnsw_label_to_id_map_.size() << " HNSW ID haritasi elemani yüklendi.");

        }
        // Eğer yeni bir index oluşturulduysa veya yüklendi ancak boşsa, veya haritalar boşsa LMDB'deki mevcut ID'leri HNSW'ye ekle
        if (hnsw_index_->get_current_elements() == 0) {
            LOG_DEFAULT(LogLevel::INFO, "SwarmVectorDB::open(): HNSW index bos, mevcut LMDB ID'leri ekleniyor.");

            MDB_txn* write_txn;
            rc = mdb_txn_begin(env_, nullptr, 0, &write_txn);
                        
            std::vector<std::string> existing_ids = get_all_ids_internal(write_txn); // Deadlock'u önlemek için internal metot çağrıldı.

            if (rc != MDB_SUCCESS) {
                LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::open(): mdb_txn_begin başarısız (hnsw populate): " << mdb_strerror(rc));
                return false;
            }

            for (const auto& id : existing_ids) {
                std::unique_ptr<CryptofigVector> cv = get_vector(id, write_txn);
                if (cv) {
                    // hnswlib::labeltype label = static_cast<hnswlib::labeltype>(std::hash<std::string>{}(id));
                    hnswlib::labeltype label = next_hnsw_label_++; 
                    std::vector<float> emb(cv->embedding.data(), cv->embedding.data() + cv->embedding.size());
                    hnsw_index_->add_item(emb, label);

                    // Haritaları güncelle
                    hnsw_label_to_id_map_[label] = id;
                    id_to_hnsw_label_map_[id] = label;

                    // Haritaları LMDB'ye kaydet (hnswlib::labeltype'ı string'e çevirerek daha güvenli MDB_val kullanımı)
                    std::string label_str_key_open = std::to_string(label);
                    MDB_val label_key_mdb, id_val_mdb;
                    label_key_mdb.mv_size = label_str_key_open.size();
                    label_key_mdb.mv_data = (void*)label_str_key_open.data();
                    id_val_mdb.mv_size = id.size();
                    id_val_mdb.mv_data = (void*)id.data();
                    rc = mdb_put(write_txn, hnsw_label_to_id_map_dbi_, &label_key_mdb, &id_val_mdb, 0);
                    if (rc != MDB_SUCCESS) LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::open(): hnsw_label_to_id_map put başarısız (label: " << label_str_key_open << ", ID: " << id << "): " << mdb_strerror(rc));

                    std::string label_str_val_open = std::to_string(label);
                    MDB_val id_key_mdb_inner, label_val_mdb;
                    id_key_mdb_inner.mv_size = id.size();
                    id_key_mdb_inner.mv_data = (void*)id.data();
                    label_val_mdb.mv_size = label_str_val_open.size();
                    label_val_mdb.mv_data = (void*)label_str_val_open.data();
                    rc = mdb_put(write_txn, id_to_hnsw_label_map_dbi_, &id_key_mdb_inner, &label_val_mdb, 0);
                    if (rc != MDB_SUCCESS) LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::open(): id_to_hnsw_label_map put başarısız (ID: " << id << ", label: " << label_str_val_open << "): " << mdb_strerror(rc));
                }
            }
            LOG_DEFAULT(LogLevel::INFO, "SwarmVectorDB::open(): LMDB'den HNSW index'e " << existing_ids.size() << " eleman eklendi.");

            // next_hnsw_label_ ve haritaların kaydedilmesi
            MDB_val next_label_key, next_label_data;
            std::string next_label_key_str_save = "next_hnsw_label";
            next_label_key.mv_size = next_label_key_str_save.length(); // Use length() for string size
            next_label_key.mv_data = (void*)next_label_key_str_save.c_str(); // Use c_str() for null-terminated string
            next_label_data.mv_size = sizeof(hnswlib::labeltype); // THIS WAS THE BUG. It should be the string size.
            next_label_data.mv_data = &next_hnsw_label_; // And this should be pointer to string data.
            rc = mdb_put(write_txn, hnsw_next_label_dbi_, &next_label_key, &next_label_data, 0);
            if (rc != MDB_SUCCESS) LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::open(): next_hnsw_label_ put başarısız: " << mdb_strerror(rc));

            rc = mdb_txn_commit(write_txn);
            if (rc != MDB_SUCCESS) {
                LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::open(): hnsw map commit başarısız: " << mdb_strerror(rc));
                mdb_txn_abort(txn); return false;
            }
        }
    }
    return true;
}

void SwarmVectorDB::close() {
    LOG_DEFAULT(LogLevel::INFO, "SwarmVectorDB::close(): Veritabanı kapatma işlemi başlatılıyor.");
    std::lock_guard<std::mutex> lock(mutex_);
    if (env_ != nullptr) {
        // HNSW index'i kapatmadan önce kaydet
        if (hnsw_index_) {
            if (!hnsw_index_->save_index(db_path_ + "/hnsw_index.bin")) {
                 LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::close(): HNSW index kaydedilemedi.");
            }
        }
        
        // Haritaları ve next_hnsw_label_'ı LMDB'ye kaydet
        MDB_txn* txn;
        int rc = mdb_txn_begin(env_, nullptr, 0, &txn);
        if (rc != MDB_SUCCESS) {
            LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::close(): mdb_txn_begin başarısız (close save maps): " << mdb_strerror(rc));
            // Yine de LMDB'yi kapatmaya devam et
        } else {
            // next_hnsw_label_ kaydet
            // next_hnsw_label_ kaydet
            MDB_val next_label_key, next_label_data;
            const std::string next_label_key_str_close = "next_hnsw_label";
            const std::string next_hnsw_label_str_close = std::to_string(next_hnsw_label_);
            next_label_key.mv_size = next_label_key_str_close.length();
            next_label_key.mv_data = (void*)next_label_key_str_close.c_str();
            next_label_data.mv_size = next_hnsw_label_str_close.length();
            next_label_data.mv_data = (void*)next_hnsw_label_str_close.c_str();
            rc = mdb_put(txn, hnsw_next_label_dbi_, &next_label_key, &next_label_data, 0);
            if (rc != MDB_SUCCESS) LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::close(): next_hnsw_label_ put başarısız: " << mdb_strerror(rc));

            // ... (rest of close method for maps and env)

            // Haritaları iterate edip kaydet (mevcut mapleri silip baştan yazmak daha güvenli olabilir)
            // Kapanışta tüm haritaları yineleyerek kaydetmek, tam tutarlılık sağlamak için önemlidir,
            // özellikle bazı güncellemeler kaçırılmışsa veya bellek içi haritalar kapanışta doğruluk kaynağı ise.
            // Her bir ekleme/silme işlemi ayrı ayrı harita girişlerini kaydettiği için,
            // burada tam bir yeniden yazma yapmak yerine, sadece next_hnsw_label_ kaydediliyordu.
            // Ancak, olası tutarsızlıkları önlemek için tüm haritaların kaydedilmesi daha sağlam bir yaklaşımdır.
            for (const auto& pair : hnsw_label_to_id_map_) {
                MDB_val key_l, val_id; // Temporary MDB_val structures
                std::string label_key_string = std::to_string(pair.first); // Convert label to string for key
                key_l.mv_size = label_key_string.size();
                key_l.mv_data = (void*)label_key_string.data();
                val_id.mv_size = pair.second.size();
                val_id.mv_data = (void*)pair.second.data();
                rc = mdb_put(txn, hnsw_label_to_id_map_dbi_, &key_l, &val_id, 0);
                if (rc != MDB_SUCCESS) LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::close(): hnsw_label_to_id_map put başarısız (close): " << mdb_strerror(rc));
            }
            for (const auto& pair : id_to_hnsw_label_map_) {
                MDB_val key_id, val_l; // Temporary MDB_val structures
                std::string label_val_string = std::to_string(pair.second); // Convert label to string for value
                key_id.mv_size = pair.first.size();
                key_id.mv_data = (void*)pair.first.data();
                val_l.mv_size = label_val_string.size();
                val_l.mv_data = (void*)label_val_string.data();
                rc = mdb_put(txn, id_to_hnsw_label_map_dbi_, &key_id, &val_l, 0);
                if (rc != MDB_SUCCESS) LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::close(): id_to_hnsw_label_map put başarısız (close): " << mdb_strerror(rc));
            }

            rc = mdb_txn_commit(txn);
            if (rc != MDB_SUCCESS) {
                LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::close(): mdb_txn_commit başarısız (close save maps): " << mdb_strerror(rc));
            } else {
                LOG_DEFAULT(LogLevel::INFO, "SwarmVectorDB::close(): HNSW harita verileri ve next_hnsw_label başarıyla kaydedildi.");
            }
        }

        if (hnsw_label_to_id_map_dbi_ != 0) {
            mdb_dbi_close(env_, hnsw_label_to_id_map_dbi_);
            hnsw_label_to_id_map_dbi_ = 0;
        }
        if (id_to_hnsw_label_map_dbi_ != 0) {
            mdb_dbi_close(env_, id_to_hnsw_label_map_dbi_);
            id_to_hnsw_label_map_dbi_ = 0;
        }
        if (hnsw_next_label_dbi_ != 0) {
            mdb_dbi_close(env_, hnsw_next_label_dbi_);
            hnsw_next_label_dbi_ = 0;
        }
        // YENİ: Q-Table DBI'larını kapat
        if (q_values_dbi_ != 0) {
            mdb_dbi_close(env_, q_values_dbi_);
            q_values_dbi_ = 0;
        }
        if (q_metadata_dbi_ != 0) {
            mdb_dbi_close(env_, q_metadata_dbi_);
            q_metadata_dbi_ = 0;
        }

        if (dbi_ != 0) {
            mdb_dbi_close(env_, dbi_);
            dbi_ = 0;
        }
        mdb_env_close(env_);
        env_ = nullptr;
        LOG_DEFAULT(LogLevel::INFO, "SwarmVectorDB: Veritabanı kapatıldı.");
    }
}

bool SwarmVectorDB::store_vector(const CryptofigVector& cv) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (env_ == nullptr) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB: Veritabanı açık değil. Vektör depolanamadı.");
        return false;
    }

    int rc;
    MDB_txn* txn;
    rc = mdb_txn_begin(env_, nullptr, 0, &txn);
    if (rc != MDB_SUCCESS) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB: mdb_txn_begin başarısız (store): " << mdb_strerror(rc));
        return false;
    }

    MDB_val key, data;
    key.mv_size = cv.id.size();
    key.mv_data = (void*)cv.id.data();

    // Serialize CryptofigVector into a byte vector
    std::vector<uint8_t> serialized_data;
    
    // 1. cryptofig_len
    size_t cryptofig_len = cv.cryptofig.size();
    serialized_data.insert(serialized_data.end(), reinterpret_cast<const uint8_t*>(&cryptofig_len), reinterpret_cast<const uint8_t*>(&cryptofig_len) + sizeof(size_t));
    // 2. cv.cryptofig
    serialized_data.insert(serialized_data.end(), cv.cryptofig.begin(), cv.cryptofig.end());
    
    // 3. cv.embedding
    // serialized_data.insert(serialized_data.end(), cv.cryptofig.begin(), cv.cryptofig.end()); // Hatalı kopyalama kaldırıldı
    const uint8_t* embedding_data_ptr = reinterpret_cast<const uint8_t*>(cv.embedding.data());
    serialized_data.insert(serialized_data.end(), embedding_data_ptr, embedding_data_ptr + (cv.embedding.size() * sizeof(float)));
   
    if (cv.embedding.size() * sizeof(float) != 128 * sizeof(float)) { // Ensure fixed embedding size
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::store_vector(): Embedding boyutu 128 float degil! ID: " << cv.id);
        mdb_txn_abort(txn);
        return false;
    }

    // 4. fisher_query_len
    size_t fisher_query_len = cv.fisher_query.size(); 
     serialized_data.insert(serialized_data.end(), reinterpret_cast<const uint8_t*>(&fisher_query_len), reinterpret_cast<const uint8_t*>(&fisher_query_len) + sizeof(size_t));
    // 5. cv.fisher_query
    serialized_data.insert(serialized_data.end(), reinterpret_cast<const uint8_t*>(cv.fisher_query.data()), reinterpret_cast<const uint8_t*>(cv.fisher_query.data()) + fisher_query_len);
    
    // 6. content_hash_len
    size_t content_hash_len = cv.content_hash.size(); 
    serialized_data.insert(serialized_data.end(), reinterpret_cast<const uint8_t*>(&content_hash_len), reinterpret_cast<const uint8_t*>(&content_hash_len) + sizeof(size_t));

    // 7. cv.content_hash
    serialized_data.insert(serialized_data.end(), reinterpret_cast<const uint8_t*>(cv.content_hash.data()), reinterpret_cast<const uint8_t*>(cv.content_hash.data()) + content_hash_len);

    // YENİ: 8. topic_len
    size_t topic_len = cv.topic.size();
    serialized_data.insert(serialized_data.end(), reinterpret_cast<const uint8_t*>(&topic_len), reinterpret_cast<const uint8_t*>(&topic_len) + sizeof(size_t));
    // YENİ: 9. cv.topic
    serialized_data.insert(serialized_data.end(), reinterpret_cast<const uint8_t*>(cv.topic.data()), reinterpret_cast<const uint8_t*>(cv.topic.data()) + topic_len);

    data.mv_size = serialized_data.size();
    data.mv_data = serialized_data.data();

    rc = mdb_put(txn, dbi_, &key, &data, 0);
    if (rc != MDB_SUCCESS) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB: mdb_put başarısız: " << mdb_strerror(rc));
        mdb_txn_abort(txn);
        return false;
    }

    // HNSW index'e de ekle (aynı transaction içinde)
    if (hnsw_index_) {
        if (!id_to_hnsw_label_map_.count(cv.id)) {
            hnswlib::labeltype current_label = next_hnsw_label_++;
            std::vector<float> emb(cv.embedding.data(), cv.embedding.data() + cv.embedding.size());
            hnsw_index_->add_item(emb, current_label);
            hnsw_label_to_id_map_[current_label] = cv.id;
            id_to_hnsw_label_map_[cv.id] = current_label;

            const std::string label_str_key_store = std::to_string(current_label);
            MDB_val label_key_mdb_store, id_val_mdb_store;
            label_key_mdb_store.mv_size = label_str_key_store.length();
            label_key_mdb_store.mv_data = (void*)label_str_key_store.c_str();
            id_val_mdb_store.mv_size = cv.id.length();
            id_val_mdb_store.mv_data = (void*)cv.id.c_str();
            rc = mdb_put(txn, hnsw_label_to_id_map_dbi_, &label_key_mdb_store, &id_val_mdb_store, 0);
            if (rc != MDB_SUCCESS) LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::store_vector(): hnsw_label_to_id_map put başarısız (label: " << label_str_key_store << ", ID: " << cv.id << "): " << mdb_strerror(rc));

            const std::string label_str_val_store = std::to_string(current_label);
            MDB_val id_key_mdb_store, label_val_mdb_store;
            id_key_mdb_store.mv_size = cv.id.length();
            id_key_mdb_store.mv_data = (void*)cv.id.c_str();
            label_val_mdb_store.mv_size = label_str_val_store.length();
            label_val_mdb_store.mv_data = (void*)label_str_val_store.c_str();
            rc = mdb_put(txn, id_to_hnsw_label_map_dbi_, &id_key_mdb_store, &label_val_mdb_store, 0);
            if (rc != MDB_SUCCESS) LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::store_vector(): id_to_hnsw_label_map put başarısız (ID: " << cv.id << ", label: " << label_str_val_store << "): " << mdb_strerror(rc));

            MDB_val next_label_key, next_label_data;
            const std::string next_label_key_str = "next_hnsw_label";
            const std::string next_hnsw_label_str_val = std::to_string(next_hnsw_label_);
            next_label_key.mv_size = next_label_key_str.size();
            next_label_key.mv_data = (void*)next_label_key_str.data();
            next_label_data.mv_size = next_hnsw_label_str_val.size();
            next_label_data.mv_data = (void*)next_hnsw_label_str_val.data();
            rc = mdb_put(txn, hnsw_next_label_dbi_, &next_label_key, &next_label_data, 0);
            if (rc != MDB_SUCCESS) LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::store_vector(): next_hnsw_label_ put başarısız: " << mdb_strerror(rc));

            LOG_DEFAULT(LogLevel::TRACE, "SwarmVectorDB: HNSW index'e vektör eklendi. ID: " << cv.id << ", Label: " << current_label);
        }
    }

    rc = mdb_txn_commit(txn);
    if (rc != MDB_SUCCESS) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB: mdb_txn_commit başarısız: " << mdb_strerror(rc));
        return false;
    }

    LOG_DEFAULT(LogLevel::TRACE, "SwarmVectorDB: Vektör başarıyla depolandı. ID: " << cv.id << ", Boyut: " << data.mv_size << " byte.");
    return true;
}

std::unique_ptr<CryptofigVector> SwarmVectorDB::get_vector(const std::string& id, MDB_txn* existing_txn) const { // Keep consistent
    std::lock_guard<std::mutex> lock(mutex_);
    if (env_ == nullptr) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB: Veritabanı açık değil. Vektör getirilemedi.");
        return nullptr;
    }

    MDB_txn* current_txn = existing_txn;
    if (!current_txn) { // Eğer dışarıdan bir transaction sağlanmadıysa, yeni bir read-only transaction başlat
        int rc = mdb_txn_begin(env_, nullptr, MDB_RDONLY, &current_txn);
        if (rc != MDB_SUCCESS) {
            LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB: mdb_txn_begin başarısız (get_vector - yeni txn): " << mdb_strerror(rc));
            return nullptr;
        }
    }

    MDB_val key, data;
    key.mv_size = id.size();
    key.mv_data = (void*)id.data();

    int rc = mdb_get(current_txn, dbi_, &key, &data);
    if (rc == MDB_NOTFOUND) {
        LOG_DEFAULT(LogLevel::TRACE, "SwarmVectorDB: Vektör bulunamadı. ID: " << id);
        if (!existing_txn) { // Kendi başlattığı transaction'ı abort et
            mdb_txn_abort(current_txn);
        }
        return nullptr;
    } else if (rc != MDB_SUCCESS) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB: mdb_get başarısız: " << mdb_strerror(rc));
        if (!existing_txn) mdb_txn_abort(current_txn); // Corrected this, was txn
        return nullptr;
    }

    auto cv = std::make_unique<CryptofigVector>();
    const uint8_t* current_ptr = reinterpret_cast<const uint8_t*>(data.mv_data);
    size_t remaining_size = data.mv_size;

    size_t offset = 0;
    
    // Yeni serileştirme sırasına göre deserileştirme
    // 1. Deserialize cryptofig_len
    size_t cryptofig_len;
    if (remaining_size < sizeof(size_t)) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB: CryptofigVector deserialization hatasi: Cryptofig boyutu verisi eksik. ID: " << id);
        if (!existing_txn) mdb_txn_abort(current_txn);
        return nullptr;
    }
    std::memcpy(&cryptofig_len, current_ptr + offset, sizeof(size_t));
    offset += sizeof(size_t);
    remaining_size -= sizeof(size_t);

    // 2. Deserialize cv.cryptofig
    if (remaining_size < cryptofig_len) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB: CryptofigVector deserialization hatasi: Cryptofig verisi eksik. ID: " << id);
        if (!existing_txn) mdb_txn_abort(current_txn); // Added this as well
        if (!existing_txn) mdb_txn_abort(current_txn);
        return nullptr;
    }
    cv->cryptofig.assign(current_ptr + offset, current_ptr + offset + cryptofig_len);
    offset += cryptofig_len;
    remaining_size -= cryptofig_len;

    // 3. Deserialize cv.embedding
    cv->embedding.resize(128);
    size_t embedding_byte_size = 128 * sizeof(float);
    // Bu kontrol önceki yamada eklenmişti.
    if (remaining_size < embedding_byte_size) { // KRİTİK EKSİK SINIR KONTROLÜ EKLENDİ
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB: CryptofigVector deserialization hatasi: Embedding verisi eksik veya bozuk. ID: " << id);
        if (!existing_txn) mdb_txn_abort(current_txn);
        return nullptr;
    }
    std::memcpy(cv->embedding.data(), current_ptr + offset, embedding_byte_size);
    offset += embedding_byte_size;
    remaining_size -= embedding_byte_size;

    // 4. Deserialize fisher_query_len
    if (remaining_size < sizeof(size_t)) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB: CryptofigVector deserialization hatasi: fisher_query uzunluk verisi eksik. ID: " << id);
        if (!existing_txn) mdb_txn_abort(current_txn);
        return nullptr;
    }
    size_t fisher_query_len;
    std::memcpy(&fisher_query_len, current_ptr + offset, sizeof(size_t));
    offset += sizeof(size_t);
    remaining_size -= sizeof(size_t);

    // 5. Deserialize cv.fisher_query
    if (remaining_size < fisher_query_len) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB: CryptofigVector deserialization hatasi: fisher_query verisi eksik. ID: " << id);
        if (!existing_txn) mdb_txn_abort(current_txn);
        return nullptr;
    }
    cv->fisher_query.assign(reinterpret_cast<const char*>(current_ptr + offset), fisher_query_len);
    offset += fisher_query_len;
    remaining_size -= fisher_query_len;

    // 6. Deserialize content_hash_len
    if (remaining_size < sizeof(size_t)) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB: CryptofigVector deserialization hatasi: content_hash uzunluk verisi eksik. ID: " << id);
        if (!existing_txn) mdb_txn_abort(current_txn);
        return nullptr;
    }
    size_t content_hash_len;
    std::memcpy(&content_hash_len, current_ptr + offset, sizeof(size_t));
    offset += sizeof(size_t);
    remaining_size -= sizeof(size_t);

    // 7. Deserialize cv.content_hash
    if (remaining_size < content_hash_len) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB: CryptofigVector deserialization hatasi: content_hash verisi eksik. ID: " << id);
        if (!existing_txn) mdb_txn_abort(current_txn);
        return nullptr;
    }
    cv->content_hash.assign(reinterpret_cast<const char*>(current_ptr + offset), content_hash_len); // This was previously the end, should be here
    offset += content_hash_len; 
    remaining_size -= content_hash_len; // After this, remaining_size should be 0 if all fields are correctly deserialized.

    // YENİ: 8. Deserialize topic_len
    size_t topic_len;
    if (remaining_size < sizeof(size_t)) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB: CryptofigVector deserialization hatasi: topic uzunluk verisi eksik. ID: " << id);
        if (!existing_txn) mdb_txn_abort(current_txn);
        return nullptr;
    }
    std::memcpy(&topic_len, current_ptr + offset, sizeof(size_t));
    offset += sizeof(size_t);
    remaining_size -= sizeof(size_t);

    // YENİ: 9. Deserialize cv.topic
    if (remaining_size < topic_len) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB: CryptofigVector deserialization hatasi: topic verisi eksik. ID: " << id);
        if (!existing_txn) mdb_txn_abort(current_txn);
        return nullptr;
    }
    cv->topic.assign(reinterpret_cast<const char*>(current_ptr + offset), topic_len);
    offset += topic_len;
    remaining_size -= topic_len;



    // Check if there's any remaining data unread, indicates serialization/deserialization mismatch
    if (remaining_size != 0) { // This check should now be more reliable
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB: CryptofigVector deserialization hatasi: Okunmayan veri kaldi (" << remaining_size << " byte). ID: " << id);
        if (!existing_txn) mdb_txn_abort(current_txn);
        return nullptr;
    }
    
    cv->id = id;
    if (!existing_txn) { // Sadece kendi başlattığı transaction'ı abort et
        mdb_txn_abort(current_txn);
    }
    LOG_DEFAULT(LogLevel::TRACE, "SwarmVectorDB: Vektör başarıyla getirildi. ID: " << id << ", Boyut: " << data.mv_size << " byte.");
    return cv;
}

bool SwarmVectorDB::delete_vector(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (env_ == nullptr) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB: Veritabanı açık değil. Vektör silinemedi.");
        return false;
    }

    int rc;
    MDB_txn* txn;
    rc = mdb_txn_begin(env_, nullptr, 0, &txn);
    if (rc != MDB_SUCCESS) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB: mdb_txn_begin başarısız (delete): " << mdb_strerror(rc));
        return false;
    }

    MDB_val key;
    key.mv_size = id.size();
    key.mv_data = (void*)id.data();

    rc = mdb_del(txn, dbi_, &key, nullptr);
    if (rc == MDB_NOTFOUND) {
        LOG_DEFAULT(LogLevel::WARNING, "SwarmVectorDB: Silinecek vektör bulunamadı. ID: " << id);
        mdb_txn_abort(txn);
        return true;
    } else if (rc != MDB_SUCCESS) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB: mdb_del başarısız: " << mdb_strerror(rc));
        mdb_txn_abort(txn);
        return false;
    }

    rc = mdb_txn_commit(txn);
    if (rc != MDB_SUCCESS) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB: mdb_txn_commit başarısız: " << mdb_strerror(rc));
        return false;
    }

    // HNSW index'ten de kaldır
    if (hnsw_index_ && id_to_hnsw_label_map_.count(id)) {
        hnswlib::labeltype label_to_remove = id_to_hnsw_label_map_[id];
        // hnswlib'de doğrudan removePoint API'si yok, yeniden indeksleme gerekebilir veya özel işaretleme.
        // Şimdilik, sadece haritalardan kaldırıyoruz. Gerçekte hnswlib'in update veya delete mantığı daha karmaşık.

        // Haritaları LMDB'den de sil
        MDB_txn* map_txn = nullptr; // Initialize to nullptr
        rc = mdb_txn_begin(env_, nullptr, 0, &map_txn);
        if (rc != MDB_SUCCESS) {
            LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::delete_vector(): mdb_txn_begin başarısız (delete map): " << mdb_strerror(rc) << ").");
            return false;
        }
        
        MDB_val label_key_del, id_key_del;
        std::string label_to_remove_str = std::to_string(label_to_remove);
        label_key_del.mv_size = label_to_remove_str.size();
        label_key_del.mv_data = (void*)label_to_remove_str.data();
        rc = mdb_del(map_txn, hnsw_label_to_id_map_dbi_, &label_key_del, nullptr);
        if (rc != MDB_SUCCESS) LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::delete_vector(): hnsw_label_to_id_map del başarısız (label: " << label_to_remove_str << "): " << mdb_strerror(rc));

        std::string id_str_del = id; // Ensure lifetime
        id_key_del.mv_size = id_str_del.size();
        id_key_del.mv_data = (void*)id_str_del.data();
        rc = mdb_del(map_txn, id_to_hnsw_label_map_dbi_, &id_key_del, nullptr);
        if (rc != MDB_SUCCESS) LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::delete_vector(): id_to_hnsw_label_map del başarısız: " << mdb_strerror(rc));

        rc = mdb_txn_commit(map_txn);
        if (rc != MDB_SUCCESS) {
            LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::delete_vector(): mdb_txn_commit başarısız (delete map): " << mdb_strerror(rc));
            return false;
        }

        hnsw_label_to_id_map_.erase(label_to_remove);
        id_to_hnsw_label_map_.erase(id);
        LOG_DEFAULT(LogLevel::TRACE, "SwarmVectorDB: HNSW index haritasindan vektör kaldirildi. ID: " << id << ", Label: " << label_to_remove);
    }

    LOG_DEFAULT(LogLevel::TRACE, "SwarmVectorDB: Vektör başarıyla silindi. ID: " << id);
    return true;
}

std::vector<std::string> SwarmVectorDB::search_similar_vectors(const std::vector<float>& query_embedding, int top_k) const {
    std::vector<std::string> result_ids;
    if (!hnsw_index_) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::search_similar_vectors(): HNSW indeksi başlatılmamış. Arama yapilamadi.");
        return result_ids;
    }

    try {
        std::vector<hnswlib::labeltype> hnsw_results = hnsw_index_->search_knn(query_embedding, top_k);
        
        for (hnswlib::labeltype label : hnsw_results) {
            if (hnsw_label_to_id_map_.count(label)) {
                result_ids.push_back(hnsw_label_to_id_map_.at(label));
            } else {
                LOG_DEFAULT(LogLevel::WARNING, "SwarmVectorDB::search_similar_vectors(): HNSW label '" << label << "' için ID bulunamadı.");
            }
        }
        LOG_DEFAULT(LogLevel::INFO, "SwarmVectorDB::search_similar_vectors(): " << result_ids.size() << " benzer vektör bulundu.");

    } catch (const std::exception& e) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::search_similar_vectors(): HNSW arama hatasi: " << e.what());
    }

    return result_ids;
}

// Private helper to get all IDs without external mutex locking and using an existing transaction
std::vector<std::string> SwarmVectorDB::get_all_ids_internal(MDB_txn* txn) const {
    std::vector<std::string> ids;
    if (txn == nullptr) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::get_all_ids_internal(): No valid MDB transaction provided.");
        return ids;
    }

    MDB_cursor* cursor;
    int rc = mdb_cursor_open(txn, dbi_, &cursor);
    if (rc != MDB_SUCCESS) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::get_all_ids_internal(): mdb_cursor_open başarısız: " << mdb_strerror(rc));
        return ids;
    }

    MDB_val key, data;
    while ((rc = mdb_cursor_get(cursor, &key, &data, MDB_NEXT)) == MDB_SUCCESS) {
        ids.emplace_back(static_cast<char*>(key.mv_data), key.mv_size);
    }

    mdb_cursor_close(cursor);

    if (rc != MDB_NOTFOUND) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::get_all_ids_internal(): mdb_cursor_get döngüsü başarısız: " << mdb_strerror(rc));
        ids.clear();
    }
    LOG_DEFAULT(LogLevel::TRACE, "SwarmVectorDB::get_all_ids_internal(): Toplam " << ids.size() << " ID getirildi.");
    return ids;
}

std::vector<std::string> SwarmVectorDB::get_all_ids() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> ids;
    if (env_ == nullptr) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::get_all_ids(): Veritabanı açık değil.");
    }

    MDB_txn* txn = nullptr; // Initialize to nullptr
    int rc = mdb_txn_begin(env_, nullptr, MDB_RDONLY, &txn);
    if (rc != MDB_SUCCESS) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::get_all_ids(): mdb_txn_begin başarısız: " << mdb_strerror(rc) << ").");
        return ids;
    }

    ids = get_all_ids_internal(txn); // Call the internal helper with the new transaction
    mdb_txn_abort(txn); // Abort the read-only transaction

    return ids;
}



// YENİ: SparseQTable kalıcılığı için metotlar
bool SwarmVectorDB::store_q_value_json(const StateKey& state_key, const std::string& action_map_json_str) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (env_ == nullptr) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::store_q_value_json(): Veritabanı açık değil. Q-değeri depolanamadı.");
        return false;
    }

    MDB_txn* txn;
    int rc = mdb_txn_begin(env_, nullptr, 0, &txn);
    if (rc != MDB_SUCCESS) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::store_q_value_json(): mdb_txn_begin başarısız: " << mdb_strerror(rc));
        return false;
    }

    MDB_val key, data;
    key.mv_size = state_key.length();
    key.mv_data = (void*)state_key.c_str();
    data.mv_size = action_map_json_str.length();
    data.mv_data = (void*)action_map_json_str.c_str();

    rc = mdb_put(txn, q_values_dbi_, &key, &data, 0);
    if (rc != MDB_SUCCESS) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::store_q_value_json(): mdb_put başarısız: " << mdb_strerror(rc));
        mdb_txn_abort(txn);
        return false;
    }

    rc = mdb_txn_commit(txn);
    if (rc != MDB_SUCCESS) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::store_q_value_json(): mdb_txn_commit başarısız: " << mdb_strerror(rc));
        return false;
    }
    LOG_DEFAULT(LogLevel::TRACE, "SwarmVectorDB::store_q_value_json(): Q-değeri başarıyla depolandı. StateKey (kısmi): " << state_key.substr(0, std::min((size_t)50, state_key.length())));
    return true;
}

std::optional<std::string> SwarmVectorDB::get_q_value_json(const StateKey& state_key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (env_ == nullptr) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::get_q_value_json(): Veritabanı açık değil. Q-değeri getirilemedi.");
        return std::nullopt;
    }

    MDB_txn* txn;
    int rc = mdb_txn_begin(env_, nullptr, MDB_RDONLY, &txn);
    if (rc != MDB_SUCCESS) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::get_q_value_json(): mdb_txn_begin başarısız: " << mdb_strerror(rc));
        return std::nullopt;
    }

    MDB_val key, data;
    key.mv_size = state_key.length();
    key.mv_data = (void*)state_key.c_str();

    rc = mdb_get(txn, q_values_dbi_, &key, &data);
    if (rc == MDB_NOTFOUND) {
        LOG_DEFAULT(LogLevel::TRACE, "SwarmVectorDB::get_q_value_json(): Q-değeri bulunamadı. StateKey (kısmi): " << state_key.substr(0, std::min((size_t)50, state_key.length())));
        mdb_txn_abort(txn);
        return std::nullopt;
    } else if (rc != MDB_SUCCESS) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::get_q_value_json(): mdb_get başarısız: " << mdb_strerror(rc));
        mdb_txn_abort(txn);
        return std::nullopt;
    }

    std::string json_str(static_cast<char*>(data.mv_data), data.mv_size);
    mdb_txn_abort(txn);
    LOG_DEFAULT(LogLevel::TRACE, "SwarmVectorDB::get_q_value_json(): Q-değeri başarıyla getirildi. StateKey (kısmi): " << state_key.substr(0, std::min((size_t)50, state_key.length())));
    return json_str;
}

bool SwarmVectorDB::delete_q_value_json(const StateKey& state_key) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (env_ == nullptr) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::delete_q_value_json(): Veritabanı açık değil. Q-değeri silinemedi.");
        return false;
    }

    MDB_txn* txn;
    int rc = mdb_txn_begin(env_, nullptr, 0, &txn);
    if (rc != MDB_SUCCESS) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::delete_q_value_json(): mdb_txn_begin başarısız: " << mdb_strerror(rc));
        return false;
    }

    MDB_val key;
    key.mv_size = state_key.length();
    key.mv_data = (void*)state_key.c_str();

    rc = mdb_del(txn, q_values_dbi_, &key, nullptr);
    if (rc == MDB_NOTFOUND) {
        LOG_DEFAULT(LogLevel::WARNING, "SwarmVectorDB::delete_q_value_json(): Silinecek Q-değeri bulunamadı. StateKey (kısmi): " << state_key.substr(0, std::min((size_t)50, state_key.length())));
        mdb_txn_abort(txn);
        return true;
    } else if (rc != MDB_SUCCESS) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::delete_q_value_json(): mdb_del başarısız: " << mdb_strerror(rc));
        mdb_txn_abort(txn);
        return false;
    }

    rc = mdb_txn_commit(txn);
    if (rc != MDB_SUCCESS) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::delete_q_value_json(): mdb_txn_commit başarısız: " << mdb_strerror(rc));
        return false;
    }
    LOG_DEFAULT(LogLevel::TRACE, "SwarmVectorDB::delete_q_value_json(): Q-değeri başarıyla silindi. StateKey (kısmi): " << state_key.substr(0, std::min((size_t)50, state_key.length())));
    return true;
}

} // namespace SwarmVectorDB
} // namespace CerebrumLux