#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>

#include "../../external/nlohmann/json.hpp" // nlohmann::json için
#include "../core/logger.h"                 // CerebrumLux Logger için
#include "../core/utils.h"                  // get_current_timestamp_str için
#include "../learning/Capsule.h"            // Mevcut Capsule yapısı için
#include "../swarm_vectordb/DataModels.h"   // CryptofigVector için
#include "../swarm_vectordb/VectorDB.h"     // SwarmVectorDB için

#include <lmdb.h> // MDB_cursor, mdb_cursor_open, mdb_cursor_get için

// Geçici olarak Eigen başlık dosyasını dahil ediyoruz, çünkü CryptofigVector constructor'ı Eigen kullanır.
// Daha sonra, bu işlem için ayrı bir embedding hesaplama modülü olacak.
#include <Eigen/Dense>

namespace CerebrumLux {
namespace Tools {

// knowledge.json'dan verileri içe aktaracak CLI aracı
class KnowledgeImporter {
public:
    KnowledgeImporter(const std::string& db_path, const std::string& json_path);
    ~KnowledgeImporter();

    // İçe aktarma işlemini başlatır
    bool import_data();

    // Veritabanındaki tüm ID'leri ve boyutları listeler
    bool list_data(); // Removed const

    // json_path_ değerine dışarıdan erişim için getter
    const std::string& get_json_path() const { return json_path_; }

private:
    std::string db_path_;
    std::string json_path_;
    SwarmVectorDB::SwarmVectorDB swarm_db_;

    // Mevcut Capsule'ı CryptofigVector'e dönüştürür (basit dönüştürme)
    SwarmVectorDB::CryptofigVector convert_capsule_to_cryptofig_vector(const Capsule& capsule) const;
};

KnowledgeImporter::KnowledgeImporter(const std::string& db_path, const std::string& json_path)
    : db_path_(db_path), json_path_(json_path), swarm_db_(db_path) {
    LOG_DEFAULT(LogLevel::INFO, "KnowledgeImporter: Başlatıldı. DB Yolu: " << db_path_ << ", JSON Yolu: " << json_path_);
}

KnowledgeImporter::~KnowledgeImporter() {
    LOG_DEFAULT(LogLevel::INFO, "KnowledgeImporter: Sonlandırıldı.");
}

bool KnowledgeImporter::import_data() {
    LOG_DEFAULT(LogLevel::INFO, "KnowledgeImporter: Veri içe aktarma başlatılıyor...");

    if (!swarm_db_.open()) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "KnowledgeImporter: SwarmVectorDB açılamadı. İçe aktarma başarısız.");
        return false;
    }

    std::ifstream ifs(json_path_);
    if (!ifs.is_open()) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "KnowledgeImporter: knowledge.json dosyası açılamadı: " << json_path_);
        swarm_db_.close();
        return false;
    }

    nlohmann::json j;
    try {
        ifs >> j;
        LOG_DEFAULT(LogLevel::INFO, "KnowledgeImporter: JSON dosyası başarıyla okundu ve ayrıştırıldı.");
    } catch (const nlohmann::json::parse_error& e) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "KnowledgeImporter: JSON ayrıştırma hatası: " << e.what() << " - " << json_path_);
        swarm_db_.close();
        return false;
    }
    ifs.close();

    if (!j.contains("active_capsules") || !j["active_capsules"].is_array()) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "KnowledgeImporter: JSON dosyasında 'active_capsules' dizisi bulunamadı veya geçersiz.");
        swarm_db_.close();
        return false;
    }

    int imported_count = 0;
    for (const auto& json_capsule : j["active_capsules"]) { // 'capsules' yerine 'active_capsules' kullanıldı
        try {
            LOG_DEFAULT(LogLevel::TRACE, "KnowledgeImporter: Kapsül işleniyor - ID: " << json_capsule.value("id", "UNKNOWN"));
            // nlohmann::json'dan mevcut Capsule yapısına dönüştür
            CerebrumLux::Capsule capsule;
            capsule.id = json_capsule.value("id", "");
            capsule.topic = json_capsule.value("topic", "");
            capsule.source = json_capsule.value("source", "");
            capsule.confidence = json_capsule.value("confidence", 0.0f);
            capsule.plain_text_summary = json_capsule.value("plain_text_summary", "");
            capsule.content = json_capsule.value("content", "");
            // embedding ve cryptofig_blob_base64 gibi alanlar şimdilik geçici olarak ele alınıyor
            // Gerçekte CryptofigProcessor'dan elde edilecek.
            capsule.cryptofig_blob_base64 = json_capsule.value("cryptofig_blob_base64", "");

            // CryptofigVector'e dönüştür
            SwarmVectorDB::CryptofigVector cv = convert_capsule_to_cryptofig_vector(capsule);
            LOG_DEFAULT(LogLevel::TRACE, "KnowledgeImporter: Vektör oluşturuldu, depolama denemesi - ID: " << cv.id);

            // SwarmVectorDB'ye depola
            if (!swarm_db_.store_vector(cv)) {
                LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "KnowledgeImporter: Vektör depolama başarısız: ID=" << cv.id);
            } else {
                imported_count++;
            }
        } catch (const std::exception& e) {
            LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "KnowledgeImporter: Kapsül işleme hatası (ID: " << json_capsule.value("id", "UNKNOWN") << "): " << e.what());
        }
        LOG_DEFAULT(LogLevel::TRACE, "KnowledgeImporter: Bir kapsül işlendi.");
    }

    swarm_db_.close();
    LOG_DEFAULT(LogLevel::INFO, "KnowledgeImporter: Veri içe aktarma tamamlandı. Toplam içe aktarılan vektör: " << imported_count);
    return true;
}

bool KnowledgeImporter::list_data() { // Removed const
    LOG_DEFAULT(LogLevel::INFO, "KnowledgeImporter: Veritabanı içeriği listeleniyor. DB Yolu: " << db_path_);

    if (!swarm_db_.open()) { // swarm_db_ artık const değil, open() çağrılabilir
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "KnowledgeImporter::list_data(): SwarmVectorDB açılamadı.");
        return false;
    }

    MDB_txn* txn;
    int rc = mdb_txn_begin(swarm_db_.get_env(), nullptr, MDB_RDONLY, &txn); // get_env() kullanıldı
    if (rc != MDB_SUCCESS) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "KnowledgeImporter::list_data(): mdb_txn_begin başarısız: " << mdb_strerror(rc));
        swarm_db_.close();
        return false;
    }

    MDB_cursor* cursor;
    rc = mdb_cursor_open(txn, swarm_db_.get_dbi(), &cursor); // get_dbi() kullanıldı
    if (rc != MDB_SUCCESS) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "KnowledgeImporter::list_data(): mdb_cursor_open başarısız: " << mdb_strerror(rc));
        mdb_txn_abort(txn);
        swarm_db_.close();
        return false;
    }

    MDB_val key, data;
    int count = 0;
    while ((rc = mdb_cursor_get(cursor, &key, &data, MDB_NEXT)) == MDB_SUCCESS) {
        std::string id_str(static_cast<char*>(key.mv_data), key.mv_size);
        LOG_DEFAULT(LogLevel::INFO, "  ID: " << id_str << ", Boyut: " << data.mv_size << " byte.");
        count++;
    }

    mdb_cursor_close(cursor);
    mdb_txn_abort(txn); // Salt okunur işlem olduğu için abort etmek güvenlidir
    swarm_db_.close();

    if (rc != MDB_NOTFOUND) { // MDB_NOTFOUND, listenin sonuna ulaştığımızı gösterir, bir hata değildir.
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "KnowledgeImporter::list_data(): mdb_cursor_get döngüsü başarısız: " << mdb_strerror(rc));
        return false;
    }

    LOG_DEFAULT(LogLevel::INFO, "KnowledgeImporter: Veritabanında toplam " << count << " vektör bulundu.");
    return true;
}

SwarmVectorDB::CryptofigVector KnowledgeImporter::convert_capsule_to_cryptofig_vector(const Capsule& capsule) const {
    // Mevcut kapsülden CryptofigVector'e basit dönüştürme.
    // cryptofig (vector<uint8_t>): Base64 decode etmeliyiz
    std::vector<uint8_t> cryptofig_bytes;
    if (!capsule.cryptofig_blob_base64.empty()) {
        // Burada gerçek bir Base64 decode işlemi yapılmalıdır.
        // Şimdilik basit bir placeholder veya hatalı bir decode.
        // CerebrumLux::Crypto::base64_decode fonksiyonu kullanılabilir.
        // Ancak bu modül crypto'ya doğrudan bağlı olmamalı, bu nedenle placeholder.
        for (char c : capsule.cryptofig_blob_base64) {
            cryptofig_bytes.push_back(static_cast<uint8_t>(c));
        }
    }

    // embedding (Eigen::VectorXf): Varsayılan 128D rastgele vektör
    // Gerçekte, kapsül içeriğinden bir embedding modeli (örn. ONNX Runtime) ile hesaplanmalıdır.
    Eigen::VectorXf embedding(128);
    for (int i = 0; i < 128; ++i) {
        embedding(i) = static_cast<float>(rand()) / static_cast<float>(RAND_MAX); // Rastgele değerler
    }

    // fisher_query: Kapsül topic'ini kullanabiliriz veya boş bırakabiliriz
    std::string fisher_query_str = "Is this data relevant to " + capsule.topic + "?";

    return SwarmVectorDB::CryptofigVector(
        cryptofig_bytes,
        embedding,
        fisher_query_str,
        capsule.id,
        capsule.id // content_hash olarak da ID kullanıldı, gerçekte hashlenmeli
    );
}

} // namespace Tools
} // namespace CerebrumLux

int main(int argc, char** argv) {
    using namespace CerebrumLux::Tools;
    KnowledgeImporter importer("data/luxdb", "knowledge.json");
    importer.import_data();
    importer.list_data();
    return 0;
}