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

// CLI için ana fonksiyon
int main(int argc, char* argv[]) {
    std::cout << "DEBUG: main() fonksiyonuna girildi." << std::endl; // İlk kontrol noktası

    // Logger'ı sadece CLI için başlat. Log dosyası adını açıkça belirleyelim.
    // CWD'ye yazılmasını bekliyoruz.
    CerebrumLux::Logger::getInstance().init(CerebrumLux::LogLevel::TRACE, "importer_log", "CLI"); // TRACE seviyesine yükseltildi, dosya adı eklendi
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "Logger başarıyla başlatıldı.");
    std::cout << "DEBUG: Logger başlatıldı." << std::endl; // Logger sonrası kontrol noktası

    if (argc < 3) {
        LOG_DEFAULT(CerebrumLux::LogLevel::ERR_CRITICAL, "Kullanım: KnowledgeImporter <db_path> <json_path>");
        std::cerr << "ERROR: Kullanım: KnowledgeImporter <db_path> <json_path>" << std::endl;
        return 1;
    }
    std::cout << "DEBUG: Argümanlar kontrol edildi." << std::endl;

    std::string db_path = argv[1];
    std::string json_path = argv[2];

    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "Argümanlar: db_path=" << db_path << ", json_path=" << json_path);
    std::cout << "DEBUG: Argümanlar: db_path=" << db_path << ", json_path=" << json_path << std::endl;

    CerebrumLux::Tools::KnowledgeImporter importer(db_path, json_path);
    std::cout << "DEBUG: KnowledgeImporter nesnesi oluşturuldu." << std::endl;
    if (importer.import_data()) {
        LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "Veri içe aktarma işlemi başarıyla tamamlandı.");
        std::cout << "DEBUG: İçe aktarma başarılı." << std::endl;
        return 0;
    } else {
        LOG_DEFAULT(CerebrumLux::LogLevel::ERR_CRITICAL, "Veri içe aktarma işlemi başarısız oldu.");
        return 1;
    }
}