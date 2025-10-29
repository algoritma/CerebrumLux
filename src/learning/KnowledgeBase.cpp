#include "KnowledgeBase.h"
#include "../core/logger.h" // LOG_DEFAULT için
#include "../core/enums.h" // LogLevel için
#include "../core/utils.h" // SafeRNG için
#include "../brain/autoencoder.h" // CryptofigAutoencoder::INPUT_DIM için
#include <fstream>
#include <algorithm> // std::remove_if için, std::transform için
#include <limits> // std::numeric_limits için
#include <chrono> // Time functions
#include <filesystem> // YENİ: std::filesystem için eklendi
#include <numeric> // std::iota için (embedding için)
#include <iomanip> // std::setw için

// Özel olarak CryptofigVector embedding dönüşümü için (şimdilik)
#include <Eigen/Dense> 

namespace CerebrumLux {

// JSON serileştirme/deserileştirme için Capsule yapısının tanımlanması
// Bu kısım KnowledgeBase.cpp'den KALDIRILDI, çünkü Capsule.h içinde zaten tanımlıdır.

// --- Yardımcı Dönüşüm Metodları ---
SwarmVectorDB::CryptofigVector KnowledgeBase::convert_capsule_to_cryptofig_vector(const Capsule& capsule) const {
    std::vector<uint8_t> cryptofig_bytes;
    // Gerçek Base64 decode işlemi Crypto modülünde olmalı, şimdilik placeholder
    if (!capsule.cryptofig_blob_base64.empty()) {
        cryptofig_bytes.reserve(capsule.cryptofig_blob_base64.length());
        for (char c : capsule.cryptofig_blob_base64) {
            cryptofig_bytes.push_back(static_cast<uint8_t>(c));
        }
    }

    // std::vector<float> to Eigen::VectorXf
    Eigen::VectorXf embedding_eigen(capsule.embedding.size());
    for (size_t i = 0; i < capsule.embedding.size(); ++i) {
        embedding_eigen(i) = capsule.embedding[i];
    }

    std::string fisher_query_str = "Is this data relevant to " + capsule.topic + "?";

    return SwarmVectorDB::CryptofigVector(
        cryptofig_bytes,
        embedding_eigen,
        fisher_query_str,
        capsule.id,
        capsule.id // content_hash olarak da ID kullanıldı, gerçekte hashlenmeli
    );
}

Capsule KnowledgeBase::convert_cryptofig_vector_to_capsule(const SwarmVectorDB::CryptofigVector& cv) const {
    Capsule capsule;
    capsule.id = cv.id;
    capsule.topic = ""; // Topic doğrudan CryptofigVector'de saklanmadığı için boş bırakıldı
    capsule.source = "SwarmVectorDB"; // Kaynak olarak DB belirtilebilir
    capsule.confidence = 1.0f; // Varsayılan güven
    capsule.plain_text_summary = cv.fisher_query; // Fisher query'yi özet olarak kullan
    capsule.content = ""; // Content doğrudan CryptofigVector'de saklanmadığı için boş
    
    // Eigen::VectorXf to std::vector<float>
    capsule.embedding.resize(cv.embedding.size());
    for (int i = 0; i < cv.embedding.size(); ++i) {
        capsule.embedding[i] = cv.embedding(i);
    }

    // cryptofig_bytes to base64 string (placeholder)
    capsule.cryptofig_blob_base64.reserve(cv.cryptofig.size());
    for (uint8_t byte : cv.cryptofig) {
        capsule.cryptofig_blob_base64 += static_cast<char>(byte);
    }

    // Diğer alanlar CryptofigVector'de saklanmadığı için boş kalır veya varsayılan değer alır
    capsule.encrypted_content = "";
    capsule.gcm_tag_base64 = "";
    capsule.encryption_iv_base64 = "";
    capsule.signature_base64 = "";
    capsule.timestamp_utc = std::chrono::system_clock::now(); // Güncel zaman
    capsule.trust_score = 1.0f;
    capsule.code_file_path = "";

    return capsule;
}

// --- KnowledgeBase Implementasyonu ---

KnowledgeBase::KnowledgeBase() : db_path_("default_lmdb_path"), swarm_db_(db_path_) { // db_path_ ve swarm_db_ başlatıldı
    LOG_DEFAULT(LogLevel::INFO, "KnowledgeBase: Varsayilan kurucu baslatildi. LMDB yolu: " << db_path_);
    if (!swarm_db_.open()) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "KnowledgeBase: Varsayilan SwarmVectorDB açılamadı. Bilgi tabanı işlevsel değil.");
    }
}

KnowledgeBase::KnowledgeBase(const std::string& db_path) : db_path_(db_path), swarm_db_(db_path_) { // Yeni kurucu
    LOG_DEFAULT(LogLevel::INFO, "KnowledgeBase Kurucusu: db_path ile baslatildi. DB Yolu: " << db_path_);
    if (!swarm_db_.open()) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "KnowledgeBase Kurucusu: SwarmVectorDB açılamadı. Bilgi tabanı işlevsel değil.");
    }
}

void KnowledgeBase::add_capsule(const Capsule& capsule) {
    // Önce kapsülün zaten var olup olmadığını kontrol et (güncelleme için)
    if (swarm_db_.get_vector(capsule.id)) { 
        LOG_DEFAULT(LogLevel::DEBUG, "KnowledgeBase: Mevcut kapsül güncellendi. ID: " << capsule.id);
    }
    SwarmVectorDB::CryptofigVector cv = convert_capsule_to_cryptofig_vector(capsule);
    if (swarm_db_.store_vector(cv)) {
         LOG_DEFAULT(LogLevel::INFO, "KnowledgeBase: New capsule added. ID: " << capsule.id);
    } else {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "KnowledgeBase: Kapsül LMDB'ye eklenemedi. ID: " << capsule.id);
    }
}

std::vector<Capsule> KnowledgeBase::semantic_search(const std::string& query, int top_k) const {
    LOG_DEFAULT(LogLevel::TRACE, "KnowledgeBase: Semantic search for query: " << query << ", Top K: " << top_k);
    std::vector<Capsule> results;

    // TODO: hnswlib entegrasyonu yapıldığında buradaki mantık değişecek, doğrudan vektör araması yapılacak.
    // Şimdilik, verimli olmasa da, tüm LMDB verilerini çekip string bazlı arama yapıyoruz.
    if (swarm_db_.is_open()) {
        std::vector<std::string> all_ids = swarm_db_.get_all_ids();
        for (const auto& id : all_ids) {
            std::unique_ptr<SwarmVectorDB::CryptofigVector> cv = swarm_db_.get_vector(id);
            if (cv) {
                Capsule current_capsule = convert_cryptofig_vector_to_capsule(*cv);
                if (current_capsule.topic.find(query) != std::string::npos || 
                    current_capsule.plain_text_summary.find(query) != std::string::npos ||
                    current_capsule.content.find(query) != std::string::npos) {
                    results.push_back(current_capsule);
                }
            }
        }
    }
        
    if (results.empty()) {
        LOG_DEFAULT(LogLevel::WARNING, "KnowledgeBase: '" << query << "' sorgusu için sonuç bulunamadı.");
    }
    return results;
}

std::vector<Capsule> KnowledgeBase::search_by_topic(const std::string& topic) const {
    LOG_DEFAULT(LogLevel::DEBUG, "KnowledgeBase: Topic'e göre arama yapılıyor: " << topic);
    std::vector<Capsule> results;
    if (swarm_db_.is_open()) {
        std::vector<std::string> all_ids = swarm_db_.get_all_ids();
        for (const auto& id : all_ids) {
            std::unique_ptr<SwarmVectorDB::CryptofigVector> cv = swarm_db_.get_vector(id);
            if (cv) {
                Capsule current_capsule = convert_cryptofig_vector_to_capsule(*cv);
                if (current_capsule.topic.find(topic) != std::string::npos) {
                    results.push_back(current_capsule);
                }
            }
        }
    }
    if (results.empty()) {
        LOG_DEFAULT(LogLevel::WARNING, "KnowledgeBase: '" << topic << "' konusu için sonuç bulunamadı.");
    }
    return results;
}

std::optional<Capsule> KnowledgeBase::find_capsule_by_id(const std::string& id) { 
    LOG_DEFAULT(LogLevel::DEBUG, "KnowledgeBase: ID'ye göre kapsül aranıyor: " << id);
    std::unique_ptr<SwarmVectorDB::CryptofigVector> cv = swarm_db_.get_vector(id); // get_vector() burada const olmayan metod içinde çağrılabilir
    if (cv) {
        return convert_cryptofig_vector_to_capsule(*cv);
    }
    LOG_DEFAULT(LogLevel::WARNING, "KnowledgeBase: ID '" << id << "' ile kapsül bulunamadı.");
    return std::nullopt;
}

void KnowledgeBase::quarantine_capsule(const std::string& id) {
    LOG_DEFAULT(LogLevel::INFO, "KnowledgeBase: Kapsül karantinaya alınıyor. ID: " << id);
    if (swarm_db_.delete_vector(id)) {
        LOG_DEFAULT(LogLevel::INFO, "KnowledgeBase: Kapsül karantinaya alındı. ID: " << id);
    } else {
        LOG_DEFAULT(LogLevel::WARNING, "KnowledgeBase: Karantinaya alınacak ID '" << id << "' ile kapsül bulunamadı.");
    }
}

void KnowledgeBase::revert_capsule(const std::string& id) {
    LOG_DEFAULT(LogLevel::INFO, "KnowledgeBase: Kapsül karantinadan geri alınıyor. ID: " << id);
    LOG_DEFAULT(LogLevel::WARNING, "KnowledgeBase::revert_capsule(): Metot henüz tam olarak uygulanmadi. ID: " << id << ".");
    // TODO: Karantinaya alınan kapsüller için ayrı bir LMDB DBI veya flag mekanizması entegre edildiğinde bu metot güncellenecek.
}

void KnowledgeBase::save(const std::string& filename) const {
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "KnowledgeBase::save(): LMDB tabanlı veritabanı doğrudan kalıcıdır. JSON dışa aktarma yapılıyor.");
    
    nlohmann::json j;
    std::vector<Capsule> all_active_capsules = get_all_capsules(); 
    j["active_capsules"] = all_active_capsules;
    j["quarantined_capsules"] = nlohmann::json::array(); // Karantinaya alınanları şimdilik boş bırakıyoruz

    std::filesystem::path current_path = std::filesystem::current_path();
    std::filesystem::path file_path = current_path / filename;
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "KnowledgeBase: Kaydedilmeye calisilan tam dosya yolu (JSON Export): " << file_path.string());

    std::ofstream o(file_path);
    if (o.is_open()) {
        o << std::setw(4) << j << std::endl;
        LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "KnowledgeBase: Bilgi tabanı JSON olarak başarıyla dışa aktarıldı: " << filename);
    } else {
        LOG_ERROR_CERR(CerebrumLux::LogLevel::ERR_CRITICAL, "KnowledgeBase: Bilgi tabanı JSON'a dışa aktarılamadı: " << file_path.string());
    }
}

void KnowledgeBase::load(const std::string& filename) {
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "KnowledgeBase::load(): LMDB tabanlı veritabanı doğrudan yüklendi (açıldı). JSON dosyasından yükleme KnowledgeImporter'a aittir.");
    if (!swarm_db_.is_open()) {
        if (!swarm_db_.open()) {
            LOG_ERROR_CERR(CerebrumLux::LogLevel::ERR_CRITICAL, "KnowledgeBase::load(): SwarmVectorDB açılamadı. Bilgi tabanı işlevsel değil.");
        }
    }

    std::filesystem::path current_path = std::filesystem::current_path();
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "KnowledgeBase: Uygulama calisma dizini: " << current_path.string());
    std::filesystem::path file_path = current_path / filename;
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "KnowledgeBase: Yuklenmeye calisilan tam dosya yolu (JSON Import denemesi): " << file_path.string());

    if (!std::filesystem::exists(file_path)) {
        LOG_DEFAULT(CerebrumLux::LogLevel::WARNING, "KnowledgeBase: JSON dosya bulunamadi: " << file_path.string() << ". LMDB tabani kullanilmaya devam ediliyor.");
        return; 
    } else {
        LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "KnowledgeBase: JSON dosya var: " << file_path.string());
    }
    
    std::ifstream i(file_path);
    if (i.is_open()) {
        try {
            nlohmann::json j;
            i >> j;
            LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "KnowledgeBase: JSON dosyasindan kapsül bilgisi bulundu ancak LMDB'ye aktarilmadi (KnowledgeImporter kullanilmalidir).");
        } catch (const nlohmann::json::exception& e) {
            LOG_ERROR_CERR(CerebrumLux::LogLevel::ERR_CRITICAL, "KnowledgeBase: JSON yükleme hatası: " << e.what() << ". LMDB tabani kullanilmaya devam ediliyor.");
        } catch (const std::exception& e) {
            LOG_ERROR_CERR(CerebrumLux::LogLevel::ERR_CRITICAL, "KnowledgeBase: Bilgi tabanı yükleme sırasında genel hata: " << e.what() << ". LMDB tabani kullanilmaya devam ediliyor.");
        }
    } else {
        LOG_DEFAULT(CerebrumLux::LogLevel::WARNING, "KnowledgeBase: JSON dosya acilamadi: " << file_path.string() << ". Dosya var ancak acilamiyor (izin veya kilit sorunu olabilir). LMDB tabani kullanilmaya devam ediliyor.");
    }
}

std::vector<Capsule> KnowledgeBase::get_all_capsules() const {
    LOG_DEFAULT(LogLevel::DEBUG, "KnowledgeBase::get_all_capsules(): Tüm aktif kapsüller LMDB'den isteniyor.");
    std::vector<Capsule> all_capsules;
    if (swarm_db_.is_open()) {
        std::vector<std::string> all_ids = swarm_db_.get_all_ids();
        for (const auto& id : all_ids) {
            std::unique_ptr<SwarmVectorDB::CryptofigVector> cv = swarm_db_.get_vector(id);
            if (cv) {
                all_capsules.push_back(convert_cryptofig_vector_to_capsule(*cv));
            }
        }
    }
    LOG_DEFAULT(LogLevel::DEBUG, "KnowledgeBase::get_all_capsules(): Toplam " << all_capsules.size() << " kapsül LMDB'den alındı.");
    return all_capsules;
}

} // namespace CerebrumLux