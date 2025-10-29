#include "VectorDB.h"
#include <iomanip> // std::hex, std::setw için
#include <sstream> // stringstream için
#include <stdexcept> // runtime_error için

#include "DataModels.h" // CryptofigVector
#include "../core/logger.h" // LOG_DEFAULT için

#include <iostream>
#include <filesystem> // std::filesystem::create_directories için
#include <stdexcept>

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

SwarmVectorDB::SwarmVectorDB(const std::string& db_path) : db_path_(db_path) {
    LOG_DEFAULT(LogLevel::INFO, "SwarmVectorDB Kurucusu: Başlatıldı. DB Yolu: " << db_path_);
    env_ = nullptr; // env_ ve dbi_ üyelerini açıkça başlat
    dbi_ = 0;
}

SwarmVectorDB::~SwarmVectorDB() {
    close();
    LOG_DEFAULT(LogLevel::INFO, "SwarmVectorDB Yıkıcısı: Çağrıldı.");
}

bool SwarmVectorDB::open() {
    std::lock_guard<std::mutex> lock(mutex_); // Thread güvenliği
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
    } catch (const fs::filesystem_error& e) { // 'filesystem_erro' -> 'filesystem_error' düzeltildi
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::open(): DB dizini oluşturma/kontrol hatası: " << e.what() << ", Yol: " << db_path_);
        return false;
    }

    // LMDB ortamı oluştur
    rc = mdb_env_create(&env_);
    if (rc != MDB_SUCCESS) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB: mdb_env_create başarısız: " << mdb_strerror(rc));
        return false; // env_ zaten nullptr, yok etmeye gerek yok
    }

    LOG_DEFAULT(LogLevel::TRACE, "SwarmVectorDB::open(): mdb_env_create başarılı.");

    // 2. Ortam boyutunu ayarla (örn: 1GB)
    // MDB_FIXEDMAP kullanılmıyorsa env_set_mapsize en az 10MB olmalıdır.
    rc = mdb_env_set_mapsize(env_, 1ULL * 1024ULL * 1024ULL * 1024ULL); // 1 GB (1GB)
    if (rc != MDB_SUCCESS) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::open(): mdb_env_set_mapsize başarısız: " << mdb_strerror(rc));
        mdb_env_close(env_); env_ = nullptr; // Başarıyla oluşturulduğu için close çağır
        return false;
    }
    LOG_DEFAULT(LogLevel::TRACE, "SwarmVectorDB::open(): mdb_env_set_mapsize başarılı (1GB).");

    // 3. Maksimum veritabanı sayısını ayarla (CryptofigVector'lar için birincil DB ve potansiyel diğerleri)
    rc = mdb_env_set_maxdbs(env_, 10); // Maksimum 10 DBI
    if (rc != MDB_SUCCESS) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::open(): mdb_env_set_maxdbs başarısız: " << mdb_strerror(rc));
        mdb_env_close(env_); env_ = nullptr; // Başarıyla oluşturulduğu için close çağır
        return false;
    }
    LOG_DEFAULT(LogLevel::TRACE, "SwarmVectorDB::open(): mdb_env_set_maxdbs başarılı (10 DB).");

    // 4. Ortamı aç (MDB_NOSUBDIR olmadan, çünkü dizini zaten oluşturduk)
    rc = mdb_env_open(env_, db_path_.c_str(), 0, 0664); // MDB_NOSUBDIR kaldırıldı, dizin zaten var.
    if (rc != MDB_SUCCESS) { // Bu satır zaten doğru gözüküyor ama consistency için MDB_SUCCESS kullandım.
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::open(): mdb_env_open başarısız: " << mdb_strerror(rc) << ", Yol: " << db_path_);
        mdb_env_close(env_); env_ = nullptr; // Başarıyla oluşturulduğu için close çağır
        return false;
    }
    LOG_DEFAULT(LogLevel::INFO, "SwarmVectorDB::open(): LMDB ortamı başarıyla açıldı. Yol: " << db_path_);

    // 5. Veritabanını aç (veya oluştur)
    MDB_txn* txn;
    rc = mdb_txn_begin(env_, nullptr, 0, &txn);
    if (rc != MDB_SUCCESS) { // Bu satır zaten doğru gözüküyor ama consistency için MDB_SUCCESS kullandım.
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::open(): mdb_txn_begin başarısız: " << mdb_strerror(rc));
        mdb_env_close(env_); env_ = nullptr; // Başarıyla oluşturulduğu için close çağır
        return false;
    }
    LOG_DEFAULT(LogLevel::TRACE, "SwarmVectorDB::open(): mdb_txn_begin başarılı.");

    // Birincil veritabanını aç (CryptofigVector'lar için)
    rc = mdb_dbi_open(txn, "cryptofig_vectors", MDB_CREATE, &dbi_); // "cryptofig_vectors" adında veritabanı
    if (rc != MDB_SUCCESS) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::open(): mdb_dbi_open 'cryptofig_vectors' başarısız: " << mdb_strerror(rc) << ", Yol: " << db_path_);
        mdb_txn_abort(txn);
        mdb_env_close(env_); env_ = nullptr; // Başarıyla oluşturulduğu için close çağır
        return false;
    }
    LOG_DEFAULT(LogLevel::TRACE, "SwarmVectorDB::open(): mdb_dbi_open 'cryptofig_vectors' başarılı.");

    rc = mdb_txn_commit(txn); // Consistency için MDB_SUCCESS kontrolü
    if (rc != MDB_SUCCESS) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB::open(): mdb_txn_commit başarısız: " << mdb_strerror(rc) << ", Yol: " << db_path_);
        mdb_env_close(env_); env_ = nullptr; // Başarıyla oluşturulduğu için close çağır
        return false;
    }
    LOG_DEFAULT(LogLevel::INFO, "SwarmVectorDB::open(): LMDB ortamı ve 'cryptofig_vectors' DB'si başarıyla açıldı.");
        return true;
}

void SwarmVectorDB::close() {
    std::lock_guard<std::mutex> lock(mutex_); // Thread güvenliği
    if (env_ != nullptr) {
        if (dbi_ != 0) { // DBI'nin varsayılan değeri 0 olabilir, eğer hiç açılmadıysa kapatmaya çalışmamalıyız
            mdb_dbi_close(env_, dbi_);
            dbi_ = 0;
        }
        mdb_env_close(env_);
        env_ = nullptr;
        LOG_DEFAULT(LogLevel::INFO, "SwarmVectorDB: Veritabanı kapatıldı.");
    }
}

bool SwarmVectorDB::store_vector(const CryptofigVector& cv) {
    std::lock_guard<std::mutex> lock(mutex_); // Thread güvenliği
    if (env_ == nullptr) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB: Veritabanı açık değil. Vektör depolanamadı.");
        return false;
    }

    int rc;
    MDB_txn* txn;
    rc = mdb_txn_begin(env_, nullptr, 0, &txn);
    if (rc != 0) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB: mdb_txn_begin başarısız (store): " << mdb_strerror(rc));
        return false;
    }

    MDB_val key, data;
    key.mv_size = cv.id.size();
    key.mv_data = (void*)cv.id.data();

    // CryptofigVector'ı tek bir byte dizisine serileştir
    std::vector<uint8_t> serialized_data;
    // cryptofig vector'ü kopyala
    serialized_data.insert(serialized_data.end(), cv.cryptofig.begin(), cv.cryptofig.end());
    // embedding vector'ünü kopyala (byte olarak)
    const uint8_t* embedding_data_ptr = reinterpret_cast<const uint8_t*>(cv.embedding.data());
    serialized_data.insert(serialized_data.end(), embedding_data_ptr, embedding_data_ptr + (cv.embedding.size() * sizeof(float)));
    // fisher_query'yi kopyala (uzunluk + string)
    size_t fisher_query_len = cv.fisher_query.size();
    serialized_data.insert(serialized_data.end(), reinterpret_cast<const uint8_t*>(&fisher_query_len), reinterpret_cast<const uint8_t*>(&fisher_query_len) + sizeof(size_t));
    serialized_data.insert(serialized_data.end(), reinterpret_cast<const uint8_t*>(cv.fisher_query.data()), reinterpret_cast<const uint8_t*>(cv.fisher_query.data()) + fisher_query_len);
    // content_hash'i kopyala
    size_t content_hash_len = cv.content_hash.size();
    serialized_data.insert(serialized_data.end(), reinterpret_cast<const uint8_t*>(&content_hash_len), reinterpret_cast<const uint8_t*>(&content_hash_len) + sizeof(size_t));
    serialized_data.insert(serialized_data.end(), reinterpret_cast<const uint8_t*>(cv.content_hash.data()), reinterpret_cast<const uint8_t*>(cv.content_hash.data()) + content_hash_len);


    data.mv_size = serialized_data.size();
    data.mv_data = serialized_data.data();

    rc = mdb_put(txn, dbi_, &key, &data, 0);
    if (rc != 0) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB: mdb_put başarısız: " << mdb_strerror(rc));
        mdb_txn_abort(txn);
        return false;
    }

    mdb_txn_commit(txn);
    LOG_DEFAULT(LogLevel::TRACE, "SwarmVectorDB: Vektör başarıyla depolandı. ID: " << cv.id << ", Boyut: " << data.mv_size << " byte.");
    return true;
}

std::unique_ptr<CryptofigVector> SwarmVectorDB::get_vector(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_); // Thread güvenliği
    if (env_ == nullptr) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB: Veritabanı açık değil. Vektör getirilemedi.");
        return nullptr;
    }

    int rc;
    MDB_txn* txn;
    rc = mdb_txn_begin(env_, nullptr, MDB_RDONLY, &txn); // Salt okunur işlem
    if (rc != 0) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB: mdb_txn_begin başarısız (get): " << mdb_strerror(rc));
        return nullptr;
    }

    MDB_val key, data;
    key.mv_size = id.size();
    key.mv_data = (void*)id.data();

    rc = mdb_get(txn, dbi_, &key, &data);
    if (rc == MDB_NOTFOUND) {
        LOG_DEFAULT(LogLevel::TRACE, "SwarmVectorDB: Vektör bulunamadı. ID: " << id);
        mdb_txn_abort(txn);
        return nullptr;
    } else if (rc != 0) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB: mdb_get başarısız: " << mdb_strerror(rc));
        mdb_txn_abort(txn);
        return nullptr;
    }

    // Veriyi deserialize et
    auto cv = std::make_unique<CryptofigVector>();
    const uint8_t* current_ptr = reinterpret_cast<const uint8_t*>(data.mv_data);
    size_t remaining_size = data.mv_size;

    // cryptofig'i deserialize et (basitçe kalan her şeyi cryptofig olarak al)
    // Gerçekte, cryptofig'in uzunluğunu da depolamalıyız.
    // Şimdilik, kalan tüm veriyi CryptofigVector'e kopyalayalım ve sonra parçalayalım
    
    // Geçici olarak tüm veriyi alıp sonra parse etme.
    // Bu kısım, CryptofigVector'ın depolama formatına sıkı sıkıya bağlıdır.
    // Önceki store_vector() metoduna göre serialize/deserialize mantığı olmalı.
    
    // Geliştirilmiş Deserialize:
    size_t offset = 0;
    
    // cryptofig'i deserialize et
    // cv->cryptofig.assign(current_ptr + offset, current_ptr + offset + cryptofig_size); // cryptofig_size'ı depolamalıyız.

    // embedding'i deserialize et (varsayılan 128D)
    cv->embedding.resize(128);
    // Güvenli bir kopyalama yapın, verinin boyutu mv_size'dan daha azsa çökebilir.
    // Minimum (remaining_size, 128 * sizeof(float)) kopyalamalıyız.
    size_t embedding_byte_size = 128 * sizeof(float);
    if (remaining_size < embedding_byte_size) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB: Embedding verisi eksik veya bozuk. ID: " << id);
        mdb_txn_abort(txn);
        return nullptr;
    }
    std::memcpy(cv->embedding.data(), current_ptr + offset, embedding_byte_size);
    offset += embedding_byte_size;
    remaining_size -= embedding_byte_size;

    // fisher_query'yi deserialize et
    if (remaining_size < sizeof(size_t)) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB: fisher_query uzunluk verisi eksik. ID: " << id);
        mdb_txn_abort(txn);
        return nullptr;
    }
    size_t fisher_query_len;
    std::memcpy(&fisher_query_len, current_ptr + offset, sizeof(size_t));
    offset += sizeof(size_t);
    remaining_size -= sizeof(size_t);

    if (remaining_size < fisher_query_len) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB: fisher_query verisi eksik. ID: " << id);
        mdb_txn_abort(txn);
        return nullptr;
    }
    cv->fisher_query.assign(reinterpret_cast<const char*>(current_ptr + offset), fisher_query_len);
    offset += fisher_query_len;
    remaining_size -= fisher_query_len;

    // content_hash'i deserialize et
    if (remaining_size < sizeof(size_t)) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB: content_hash uzunluk verisi eksik. ID: " << id);
        mdb_txn_abort(txn);
        return nullptr;
    }
    size_t content_hash_len;
    std::memcpy(&content_hash_len, current_ptr + offset, sizeof(size_t));
    offset += sizeof(size_t);
    remaining_size -= sizeof(size_t);

    if (remaining_size < content_hash_len) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB: content_hash verisi eksik. ID: " << id);
        mdb_txn_abort(txn);
        return nullptr;
    }
    cv->content_hash.assign(reinterpret_cast<const char*>(current_ptr + offset), content_hash_len);
    offset += content_hash_len;
    remaining_size -= content_hash_len;

    // cryptofig'i deserialize et (geri kalan her şey cryptofig olmalı)
    cv->cryptofig.assign(current_ptr + offset, current_ptr + offset + remaining_size);


    cv->id = id; // ID'yi ayarla
    mdb_txn_abort(txn); // Salt okunur işlem olduğu için abort etmek güvenlidir
    LOG_DEFAULT(LogLevel::TRACE, "SwarmVectorDB: Vektör başarıyla getirildi. ID: " << id << ", Boyut: " << data.mv_size << " byte.");
    return cv;
}

bool SwarmVectorDB::delete_vector(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_); // Thread güvenliği
    if (env_ == nullptr) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB: Veritabanı açık değil. Vektör silinemedi.");
        return false;
    }

    int rc;
    MDB_txn* txn;
    rc = mdb_txn_begin(env_, nullptr, 0, &txn);
    if (rc != 0) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB: mdb_txn_begin başarısız (delete): " << mdb_strerror(rc));
        return false;
    }

    MDB_val key;
    key.mv_size = id.size();
    key.mv_data = (void*)id.data();

    rc = mdb_del(txn, dbi_, &key, nullptr); // nullptr, herhangi bir data değerini siler
    if (rc == MDB_NOTFOUND) {
        LOG_DEFAULT(LogLevel::WARNING, "SwarmVectorDB: Silinecek vektör bulunamadı. ID: " << id);
        mdb_txn_abort(txn);
        return true; // Zaten yoksa başarı sayılır
    } else if (rc != 0) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SwarmVectorDB: mdb_del başarısız: " << mdb_strerror(rc));
        mdb_txn_abort(txn);
        return false;
    }

    mdb_txn_commit(txn);
    LOG_DEFAULT(LogLevel::TRACE, "SwarmVectorDB: Vektör başarıyla silindi. ID: " << id);
    return true;
}


} // namespace SwarmVectorDB
} // namespace CerebrumLux