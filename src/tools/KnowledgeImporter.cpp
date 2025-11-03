#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <random> // std::mt19937, std::uniform_real_distribution için

#include "../external/nlohmann/json.hpp" // nlohmann::json için
#include "../core/logger.h"                 // CerebrumLux Logger için
#include "../core/utils.h"                  // get_current_timestamp_str için
#include "../learning/Capsule.h"            // Mevcut Capsule yapısı için
#include "../swarm_vectordb/DataModels.h"   // CryptofigVector için
#include "../swarm_vectordb/VectorDB.h"     // SwarmVectorDB için
#include "../learning/KnowledgeBase.h"      // KnowledgeBase için
#include "../communication/natural_language_processor.h" // NaturalLanguageProcessor için


#include <lmdb.h> // MDB_cursor, mdb_cursor_open, mdb_cursor_get için

// Geçici olarak Eigen başlık dosyasını dahil ediyoruz, çünkü CryptofigVector constructor'ı Eigen kullanır.
// Daha sonra, bu işlem için ayrı bir embedding hesaplama modülü olacak.
#include <Eigen/Dense>

// Windows için konsol kodlamasını ayarlamak için (isteğe bağlı)
#ifdef _WIN32
#include <Windows.h>
#endif

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
    bool list_data(); 

    // Veritabanında semantik arama yapar
    bool perform_semantic_search(const std::string& query, int top_k); // CLI için string sorguyu korur

private:
    std::string db_path_;
    std::string json_path_; // Import modu için json dosyasının yolu
    CerebrumLux::KnowledgeBase m_knowledge_base; // KnowledgeBase'in yeni üyesi olarak eklendi, artık swarm_db_ yok

    // json_path_ değerine dışarıdan erişim için getter
    const std::string& get_json_path() const { return json_path_; } // No change needed

    // Mevcut Capsule'ı CryptofigVector'e dönüştürür (basit dönüştürme)
    SwarmVectorDB::CryptofigVector convert_capsule_to_cryptofig_vector(const Capsule& capsule) const;
};

KnowledgeImporter::KnowledgeImporter(const std::string& db_path, const std::string& json_path)
    : db_path_(db_path), json_path_(json_path), m_knowledge_base(db_path) { // m_knowledge_base doğrudan üye olarak başlatıldı
    LOG_DEFAULT(LogLevel::INFO, "KnowledgeImporter: Başlatıldı. DB Yolu: " << db_path_ << ", JSON Yolu: " << json_path_);
}

KnowledgeImporter::~KnowledgeImporter() {
    LOG_DEFAULT(LogLevel::INFO, "KnowledgeImporter: Sonlandırıldı.");
}

bool KnowledgeImporter::import_data() {
    LOG_DEFAULT(LogLevel::INFO, "KnowledgeImporter: Veri içe aktarma başlatılıyor...");

    // KnowledgeBase::import_from_json metodu artık SwarmVectorDB'yi kendi içinde açıp kapatıyor.
    // Bu sayede KnowledgeImporter daha temiz bir arayüze sahip olur.
    // m_knowledge_base.import_from_json çağrısı yapılmalı

    m_knowledge_base.import_from_json(json_path_); // Yeni import metodu çağrıldı

    std::ifstream ifs(json_path_);
    if (!ifs.is_open()) { // No change needed
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "KnowledgeImporter: knowledge.json dosyası açılamadı: " << json_path_);
        // import_from_json zaten kapatıyor, burada tekrar kapatmaya gerek yok.
        return false;
    }

    nlohmann::json j;
    try {
        ifs >> j;
        LOG_DEFAULT(LogLevel::INFO, "KnowledgeImporter: JSON dosyası başarıyla okundu ve ayrıştırıldı.");
    } catch (const nlohmann::json::parse_error& e) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "KnowledgeImporter: JSON ayrıştırma hatası: " << e.what() << " - " << json_path_);
        // import_from_json zaten kapatıyor, burada tekrar kapatmaya gerek yok.
        return false;
    }
    ifs.close();

    if (!j.contains("active_capsules") || !j["active_capsules"].is_array()) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "KnowledgeImporter: JSON dosyasında 'active_capsules' dizisi bulunamadı veya geçersiz.");
        // import_from_json zaten kapatıyor, burada tekrar kapatmaya gerek yok.
        return false;
    }

    int imported_count = 0;
    for (const auto& json_capsule : j["active_capsules"]) {
        try {
            CerebrumLux::Capsule capsule;
            capsule.id = json_capsule.value("id", "");
            capsule.topic = json_capsule.value("topic", "");
            capsule.source = json_capsule.value("source", "");
            capsule.content = json_capsule.value("content", "");
            capsule.cryptofig_blob_base64 = json_capsule.value("cryptofig_blob_base64", "");

            m_knowledge_base.add_capsule(capsule); 
            imported_count++;
        } catch (const std::exception& e) {
            LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "KnowledgeImporter: Kapsül işleme hatası (ID: " << json_capsule.value("id", "UNKNOWN") << "): " << e.what());
        }
        LOG_DEFAULT(LogLevel::TRACE, "KnowledgeImporter: Bir kapsül işlendi.");
    }

    LOG_DEFAULT(LogLevel::INFO, "KnowledgeImporter: Veri içe aktarma tamamlandı. Toplam içe aktarılan vektör: " << imported_count);
    return true;
}

bool KnowledgeImporter::list_data() {
    LOG_DEFAULT(LogLevel::INFO, "KnowledgeImporter: Veritabanı içeriği listeleniyor. DB Yolu: " << db_path_);

    if (!m_knowledge_base.get_swarm_db().open()) { 
        std::cerr << "Hata: SwarmVectorDB veritabanı (list_data) açılamadı. Lütfen dosya yollarını ve izinleri kontrol edin." << std::endl;
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "KnowledgeImporter::list_data(): SwarmVectorDB açılamadı.");
        return false;
    }

    MDB_txn* txn;
    int rc; // rc bildirimi eklendi
    rc = mdb_txn_begin(m_knowledge_base.get_swarm_db().get_env(), nullptr, MDB_RDONLY, &txn); 
    if (rc != MDB_SUCCESS) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "KnowledgeImporter::list_data(): mdb_txn_begin başarısız: " << mdb_strerror(rc));
        m_knowledge_base.get_swarm_db().close(); 
        return false;
    }

    MDB_cursor* cursor;
    rc = mdb_cursor_open(txn, m_knowledge_base.get_swarm_db().get_dbi(), &cursor); 
    if (rc != MDB_SUCCESS) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "KnowledgeImporter::list_data(): mdb_cursor_open başarısız: " << mdb_strerror(rc));
        mdb_txn_abort(txn);
        m_knowledge_base.get_swarm_db().close(); 
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
    mdb_txn_abort(txn);
    m_knowledge_base.get_swarm_db().close(); 

    if (rc != MDB_NOTFOUND) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "KnowledgeImporter::list_data(): mdb_cursor_get döngüsü başarısız: " << mdb_strerror(rc));
        return false;
    }

    LOG_DEFAULT(LogLevel::INFO, "KnowledgeImporter: Veritabanında toplam " << count << " vektör bulundu.");
    return true;
}

bool KnowledgeImporter::perform_semantic_search(const std::string& query, int top_k) {
    LOG_DEFAULT(LogLevel::INFO, "KnowledgeImporter: Semantik arama başlatılıyor. Sorgu: '" << query << "', Top K: " << top_k);

    // Arama işlemi için veritabanını aç
    if (!m_knowledge_base.get_swarm_db().open()) { 
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "KnowledgeImporter::perform_semantic_search(): SwarmVectorDB açılamadı. Arama yapılamadı.");
        return false;
    }

    // Sorgu metni için embedding hesapla (statik NaturalLanguageProcessor metodunu kullan)
    std::vector<float> query_embedding = CerebrumLux::NaturalLanguageProcessor::generate_text_embedding(query);
    if (query_embedding.empty()) { 
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "KnowledgeImporter::perform_semantic_search(): Sorgu embedding'i olusturulamadi. Arama yapilamadi.");
        m_knowledge_base.get_swarm_db().close(); 
        return false;
    }

    std::vector<std::string> similar_ids = m_knowledge_base.get_swarm_db().search_similar_vectors(query_embedding, top_k);

    LOG_DEFAULT(LogLevel::INFO, "KnowledgeImporter: Semantik arama sonuçları (Top " << top_k << "):");
    if (similar_ids.empty()) {
        LOG_DEFAULT(LogLevel::INFO, "  Hiç benzer vektör bulunamadı.");
    } else {
        for (const auto& id : similar_ids) {
            std::optional<CerebrumLux::Capsule> capsule = m_knowledge_base.find_capsule_by_id(id);
            if (capsule) {
                LOG_DEFAULT(LogLevel::INFO, "  ID: " << capsule->id << ", Konu: " << capsule->topic << ", Özet: " << capsule->plain_text_summary.substr(0, std::min((size_t)100, capsule->plain_text_summary.length())) << "...");
            } else {
                LOG_DEFAULT(LogLevel::WARNING, "  ID: " << id << " için kapsül bulunamadı.");
            }
        }
    }

    m_knowledge_base.get_swarm_db().close(); 
    LOG_DEFAULT(LogLevel::INFO, "KnowledgeImporter: Semantik arama başarıyla tamamlandı.");
    return true;
}

SwarmVectorDB::CryptofigVector KnowledgeImporter::convert_capsule_to_cryptofig_vector(const Capsule& capsule) const {
    std::vector<uint8_t> cryptofig_bytes;
    if (!capsule.cryptofig_blob_base64.empty()) {
        for (char c : capsule.cryptofig_blob_base64) {
            cryptofig_bytes.push_back(static_cast<uint8_t>(c));
        }
    }

    const int EMBEDDING_DIM = 128;
    Eigen::VectorXf embedding(EMBEDDING_DIM);
    if (capsule.embedding.size() == EMBEDDING_DIM) {
        for (int i = 0; i < EMBEDDING_DIM; ++i) {
            embedding(i) = capsule.embedding[i];
        }
    } else {
        // Boyut uyuşmuyorsa veya boşsa, sıfırla doldur
        for (int i = 0; i < EMBEDDING_DIM; ++i) {
            embedding(i) = 0.0f;
        }
        if (!capsule.embedding.empty()) {
             LOG_DEFAULT(LogLevel::WARNING, "KnowledgeImporter: Capsule ID " << capsule.id << " için embedding boyutu (" << capsule.embedding.size() << ") beklenen (" << EMBEDDING_DIM << ") ile uyuşmuyor. Sıfırlarla dolduruldu.");
        }
    }

    std::string fisher_query_str = "Is this data relevant to " + capsule.topic + "?";

    return SwarmVectorDB::CryptofigVector(
        cryptofig_bytes,
        embedding,
        fisher_query_str,
        capsule.topic,
        capsule.id,
        capsule.id
    );
}

} // namespace Tools
} // namespace CerebrumLux

// CLI için ana fonksiyon
int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    CerebrumLux::Logger::getInstance().init(CerebrumLux::LogLevel::TRACE, "importer_log.txt", "CLI");
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "KnowledgeImporter CLI başlatıldı.");

    if (argc < 2) {
        LOG_DEFAULT(CerebrumLux::LogLevel::ERR_CRITICAL, "Hatalı kullanım. Veritabanı yolu (db_path) belirtilmedi.");
        std::cerr << "Hata: Veritabanı yolu belirtilmedi." << std::endl;
        std::cerr << "Kullanım 1 (Import): " << argv[0] << " <db_path> <json_path>" << std::endl;
        std::cerr << "Kullanım 2 (List):   " << argv[0] << " <db_path> -list" << std::endl;
        std::cerr << "Kullanım 3 (Search): " << argv[0] << " <db_path> -search <sorgu>" << std::endl;
        return 1;
    }

    std::string db_path = argv[1];

    // Durum 1: Listeleme komutu
    if (argc == 3 && std::string(argv[2]) == "-list") {
        LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "Mod: Veritabanı Listeleme. DB Yolu: " << db_path);
        CerebrumLux::Tools::KnowledgeImporter importer(db_path, "");
        if (!importer.list_data()) {
            LOG_DEFAULT(CerebrumLux::LogLevel::ERR_CRITICAL, "Veritabanı listeleme işlemi başarısız oldu.");
            return 1;
        }

    // Durum 2: Arama komutu
    } else if (argc == 4 && std::string(argv[2]) == "-search") {
        std::string query = argv[3];
        LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "Mod: Semantik Arama. DB Yolu: " << db_path << ", Sorgu: " << query);
        CerebrumLux::Tools::KnowledgeImporter importer(db_path, "");
        // Arama fonksiyonu const olduğu için, db'yi açıp kapatması gerekir.
        // Bu nedenle, önce db'yi açan bir metodun çağrılması daha iyi bir tasarım olabilir.
        // Şimdilik, arama fonksiyonunun kendisi db'yi yönetmeli.
        if (!importer.perform_semantic_search(query, 5)) { // Top K=5
            LOG_DEFAULT(CerebrumLux::LogLevel::ERR_CRITICAL, "Semantik arama işlemi başarısız oldu.");
            return 1;
        }

    // Durum 3: İçe aktarma komutu
    } else if (argc == 3) {
        std::string json_path = argv[2];
        LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "Mod: JSON İçe Aktarma. DB Yolu: " << db_path << ", JSON Yolu: " << json_path);
        CerebrumLux::Tools::KnowledgeImporter importer(db_path, json_path);
        if (!importer.import_data()) {
            LOG_DEFAULT(CerebrumLux::LogLevel::ERR_CRITICAL, "Veri içe aktarma işlemi başarısız oldu.");
            return 1;
        }

    // Hatalı kullanım
    } else {
        LOG_DEFAULT(CerebrumLux::LogLevel::ERR_CRITICAL, "Hatalı veya eksik argümanlar.");
        std::cerr << "Hata: Hatalı veya eksik argümanlar." << std::endl;
        std::cerr << "Kullanım 1 (Import): " << argv[0] << " <db_path> <json_path>" << std::endl;
        std::cerr << "Kullanım 2 (List):   " << argv[0] << " <db_path> -list" << std::endl;
        std::cerr << "Kullanım 3 (Search): " << argv[0] << " <db_path> -search <sorgu>" << std::endl;
        return 1;
    }

    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "İşlem başarıyla tamamlandı.");
    return 0;
}