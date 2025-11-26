#include "KnowledgeBase.h"
#include "../core/logger.h" // LOG_DEFAULT için
#include "../core/enums.h" // LogLevel için
#include "../core/utils.h" // SafeRNG için
#include "../brain/autoencoder.h" // CryptofigAutoencoder::INPUT_DIM için
#include <fstream>
#include <algorithm> // std::min, std::remove_if, std::transform için
#include <limits> // std::numeric_limits için
#include <chrono> // Time functions
#include <filesystem> // std::filesystem için eklendi
#include <numeric> // std::iota için (embedding için)
#include <iomanip> // std::setw için

#ifdef _WIN32
#include <Windows.h>
#else
#include <QCoreApplication>
#endif

// Özel olarak CryptofigVector embedding dönüşümü için
#include <Eigen/Dense> 

namespace CerebrumLux {

// Düzeltme: Uygulamanın çalıştığı dizini platformdan bağımsız olarak bulan yardımcı fonksiyon.
// Bu, 'get_application_directory' was not declared in this scope hatasını çözer.
std::filesystem::path get_application_directory() {
    std::filesystem::path app_dir;
#ifdef _WIN32
    char path[MAX_PATH] = {0};
    if (GetModuleFileNameA(NULL, path, MAX_PATH) == 0) {
        LOG_DEFAULT(LogLevel::ERR_CRITICAL, "GetModuleFileNameA failed. Fallback to current_path().");
        return std::filesystem::current_path();
    }
    app_dir = std::filesystem::path(path).parent_path();
#else
    app_dir = QCoreApplication::applicationDirPath().toStdString();
#endif
    return app_dir;
}
// --- Yardımcı Dönüşüm Metodları ---
SwarmVectorDB::CryptofigVector KnowledgeBase::convert_capsule_to_cryptofig_vector(const Capsule& capsule) const {
    std::vector<uint8_t> cryptofig_bytes;
    if (!capsule.cryptofig_blob_base64.empty()) {
        cryptofig_bytes.reserve(capsule.cryptofig_blob_base64.length());
        for (char c : capsule.cryptofig_blob_base64) {
            cryptofig_bytes.push_back(static_cast<uint8_t>(c));
        }
    }

    const int EMBEDDING_DIM = CerebrumLux::CryptofigAutoencoder::INPUT_DIM;
    Eigen::VectorXf embedding_eigen(EMBEDDING_DIM);

    if (capsule.embedding.size() != EMBEDDING_DIM) {
        LOG_DEFAULT(LogLevel::WARNING, "KnowledgeBase: Capsule ID " << capsule.id 
                    << " için embedding boyutu (" << capsule.embedding.size()
                    << ") beklenen (" << EMBEDDING_DIM << ") ile uyuşmuyor. Bellekte düzeltiliyor.");
    }
    
    for (int i = 0; i < EMBEDDING_DIM; ++i) {
        if (i < capsule.embedding.size()) {
            embedding_eigen(i) = capsule.embedding[i];
        } else {
            embedding_eigen(i) = 0.0f; // Eksik elemanları sıfırla
        }
    }
    
    std::string fisher_query_str = "Is this data relevant to " + capsule.topic + "?";
    return SwarmVectorDB::CryptofigVector(
        cryptofig_bytes,
        embedding_eigen,
        fisher_query_str,
        capsule.topic,
        capsule.id,
        capsule.id
    );
}

Capsule KnowledgeBase::convert_cryptofig_vector_to_capsule(const SwarmVectorDB::CryptofigVector& cv) const {
    Capsule capsule;
    capsule.id = cv.id;
    capsule.topic = cv.topic;
    capsule.source = "SwarmVectorDB";
    capsule.confidence = 1.0f;
    capsule.plain_text_summary = cv.fisher_query;

    // DÜZELTME: Kapsül içeriğini LMDB'den çek
    std::optional<std::string> content_opt = m_swarm_db.get_capsule_content(cv.id);
    if (content_opt) {
        capsule.content = *content_opt;
    } else {
        capsule.content = ""; 
    }
    
    capsule.embedding.resize(cv.embedding.size());
    for (int i = 0; i < cv.embedding.size(); ++i) {
        capsule.embedding[i] = cv.embedding(i);
    }

    capsule.cryptofig_blob_base64.reserve(cv.cryptofig.size());
    for (uint8_t byte : cv.cryptofig) {
        capsule.cryptofig_blob_base64 += static_cast<char>(byte);
    }

    capsule.encrypted_content = "";
    capsule.gcm_tag_base64 = "";
    capsule.encryption_iv_base64 = "";
    capsule.signature_base64 = "";
    capsule.timestamp_utc = std::chrono::system_clock::now();
    capsule.trust_score = 1.0f;
    capsule.code_file_path = "";

    return capsule;
}

// --- KnowledgeBase Implementasyonu ---

KnowledgeBase::KnowledgeBase() 
    : db_path_("../data/CerebrumLux_lmdb_db"), m_swarm_db(db_path_)
{
    LOG_DEFAULT(LogLevel::INFO, "KnowledgeBase: Varsayilan kurucu baslatildi. LMDB yolu: " << db_path_);
    if (!m_swarm_db.open()) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "KnowledgeBase: SwarmVectorDB baslangicta acilamadi! Devam edemiyor.");
    } else {
        LOG_DEFAULT(LogLevel::INFO, "KnowledgeBase: SwarmVectorDB basariyla acildi: " << db_path_);
    }
}

KnowledgeBase::~KnowledgeBase() { 
    LOG_DEFAULT(LogLevel::INFO, "KnowledgeBase: Yikici cagri yapiliyor. SwarmVectorDB kapatiliyor.");
    m_swarm_db.close(); // Uygulama kapanırken veritabanını kapat
    LOG_DEFAULT(LogLevel::INFO, "KnowledgeBase: SwarmVectorDB kapatildi.");
}

KnowledgeBase::KnowledgeBase(const std::string& db_path) 
    : db_path_(db_path), m_swarm_db(db_path_)
{ 
    LOG_DEFAULT(LogLevel::INFO, "KnowledgeBase: Parametreli kurucu baslatildi. LMDB yolu: " << db_path_);
    if (!m_swarm_db.open()) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "KnowledgeBase: SwarmVectorDB baslangicta acilamadi! Devam edemiyor.");
    } else {
        LOG_DEFAULT(LogLevel::INFO, "KnowledgeBase: SwarmVectorDB basariyla acildi: " << db_path_);
    }
}

void KnowledgeBase::add_capsule(const Capsule& capsule) {
    // --- KENDİ KENDİNİ İYİLEŞTİRME (SELF-HEALING) MEKANİZMASI ---
    Capsule corrected_capsule = capsule;
    if (corrected_capsule.embedding.size() != CerebrumLux::CryptofigAutoencoder::INPUT_DIM) {
        LOG_DEFAULT(LogLevel::TRACE, "KnowledgeBase: Capsule ID " << corrected_capsule.id << " için embedding boyutu (" << corrected_capsule.embedding.size() << ") beklenen (" << CerebrumLux::CryptofigAutoencoder::INPUT_DIM << ") ile uyuşmuyor. Boyut düzeltiliyor.");
        corrected_capsule.embedding.resize(CerebrumLux::CryptofigAutoencoder::INPUT_DIM, 0.0f);
    }

    if (m_swarm_db.get_vector(capsule.id)) {
        LOG_DEFAULT(LogLevel::DEBUG, "KnowledgeBase: Mevcut kapsül güncellendi. ID: " << capsule.id);
    }
    SwarmVectorDB::CryptofigVector cv = convert_capsule_to_cryptofig_vector(corrected_capsule);
    
    bool vec_ok = m_swarm_db.store_vector(cv);
    bool content_ok = m_swarm_db.store_capsule_content(capsule.id, capsule.content);

    if (vec_ok && content_ok) {
         LOG_DEFAULT(LogLevel::INFO, "KnowledgeBase: Kapsül ve içerik başarıyla eklendi. ID: " << capsule.id);
    } else {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "KnowledgeBase: Kapsül tam olarak eklenemedi (Vec: " << vec_ok << ", Cont: " << content_ok << "). ID: " << capsule.id);
    }
}

std::vector<Capsule> KnowledgeBase::semantic_search(const std::vector<float>& query_embedding, int top_k) const {
    LOG_DEFAULT(LogLevel::TRACE, "KnowledgeBase: Semantic search initiated with embedding. Top K: " << top_k);
    std::vector<Capsule> results;

    if (!m_swarm_db.is_open()) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "KnowledgeBase: SwarmVectorDB acik degil. Semantik arama yapilamadi.");
        return results;
    }

    if (query_embedding.empty()) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "KnowledgeBase: semantic_search'e boş sorgu embedding'i verildi. Arama yapilamadi.");
        return results;
    }

    std::vector<std::string> similar_ids = m_swarm_db.search_similar_vectors(query_embedding, top_k);
    for (const auto& id : similar_ids) {
        std::optional<Capsule> capsule_opt = find_capsule_by_id(id); 
        if (capsule_opt) {
            results.push_back(*capsule_opt);
        }
    }
    LOG_DEFAULT(LogLevel::INFO, "KnowledgeBase: Embedding ile arama için " << results.size() << " semantik sonuç bulundu.");
    return results;
}

std::vector<Capsule> KnowledgeBase::search_by_topic(const std::vector<float>& topic_embedding, int top_k) const {
    LOG_DEFAULT(LogLevel::DEBUG, "KnowledgeBase: Topic embedding ile arama yapılıyor. Top K: " << top_k);
    return semantic_search(topic_embedding, top_k); // Doğrudan embedding ile arama
}

std::optional<Capsule> KnowledgeBase::find_capsule_by_id(const std::string& id) const {
    LOG_DEFAULT(LogLevel::DEBUG, "KnowledgeBase: ID'ye göre kapsül aranıyor: " << id);
    std::unique_ptr<SwarmVectorDB::CryptofigVector> cv = m_swarm_db.get_vector(id);
    if (cv) {
        return convert_cryptofig_vector_to_capsule(*cv);
    }
    LOG_DEFAULT(LogLevel::WARNING, "KnowledgeBase: ID '" << id << "' ile kapsül bulunamadı.");
    return std::nullopt;
}

void KnowledgeBase::quarantine_capsule(const std::string& id) {
    LOG_DEFAULT(LogLevel::INFO, "KnowledgeBase: Kapsül karantinaya alınıyor. ID: " << id); 
    if (m_swarm_db.delete_vector(id)) {
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

void KnowledgeBase::export_to_json(const std::string& filename) const {
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "KnowledgeBase::export_to_json(): Bilgi tabanı JSON olarak dışa aktarılıyor.");

    nlohmann::json j;
    std::vector<Capsule> all_active_capsules = get_all_capsules(); 
    j["active_capsules"] = all_active_capsules;

    // Düzeltme: Dosya yolunu, çalıştırılabilir dosyanın konumuna göre belirle.
    // Bu, uygulamanın 'build' klasöründen çalıştırıldığında bile doğru 'data' klasörünü bulmasını sağlar.
    std::filesystem::path app_dir = get_application_directory();
    std::filesystem::path data_dir = app_dir.parent_path() / "data";
    std::filesystem::path file_path = data_dir / filename;
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "KnowledgeBase: Kaydedilmeye calisilan tam dosya yolu (JSON Export): " << file_path.string());

    std::ofstream o(file_path);
    if (o.is_open()) {
        o << std::setw(4) << j << std::endl;
        LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "KnowledgeBase: Bilgi tabanı JSON olarak başarıyla dışa aktarıldı: " << file_path.string());
    } else {
        LOG_ERROR_CERR(CerebrumLux::LogLevel::ERR_CRITICAL, "KnowledgeBase: Bilgi tabanı JSON'a dışa aktarılamadı (dosya açılamadı): " << file_path.string());
    }
}

void KnowledgeBase::import_from_json(const std::string& filename) {
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "KnowledgeBase::import_from_json(): JSON dosyasından kapsüller içe aktarılıyor.");
    // Düzeltme: Dosya yolunu, çalıştırılabilir dosyanın konumuna göre belirle.
    // Bu, uygulamanın 'build' klasöründen çalıştırıldığında bile doğru 'data' klasörünü bulmasını sağlar.
    std::filesystem::path app_dir = get_application_directory();
    std::filesystem::path data_dir = app_dir.parent_path() / "data";
    std::filesystem::path file_path = data_dir / filename;
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "KnowledgeBase: Yuklenmeye calisilan tam dosya yolu (JSON Import denemesi): " << file_path.string());

    if (!std::filesystem::exists(file_path)) {
        LOG_DEFAULT(CerebrumLux::LogLevel::WARNING, "KnowledgeBase: JSON dosya bulunamadi: " << file_path.string() << ". İçe aktarma atlandı.");
        return; 
    }

    std::ifstream i(file_path);
    if (i.is_open()) {
        LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "KnowledgeBase: JSON dosya var: " << file_path.string());
        try {
            nlohmann::json j;
            i >> j;
            if (j.contains("active_capsules") && j["active_capsules"].is_array()) {
                for (const auto& json_capsule : j["active_capsules"]) {
                    CerebrumLux::Capsule capsule = json_capsule.get<CerebrumLux::Capsule>();
                    add_capsule(capsule); // Her kapsülü KnowledgeBase'e ekle
                }
                LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "KnowledgeBase: JSON dosyasindan kapsüller başarıyla içe aktarıldı: " << file_path.string());
            } else {
                LOG_DEFAULT(CerebrumLux::LogLevel::WARNING, "KnowledgeBase: JSON dosyasinda 'active_capsules' dizisi bulunamadı veya geçersiz. İçe aktarma atlandı.");
            }
        } catch (const nlohmann::json::exception& e) {
            LOG_ERROR_CERR(CerebrumLux::LogLevel::ERR_CRITICAL, "KnowledgeBase: JSON ayrıştırma hatası: " << e.what() << ". İçe aktarma başarısız.");
        } catch (const std::exception& e) {
            LOG_ERROR_CERR(CerebrumLux::LogLevel::ERR_CRITICAL, "KnowledgeBase: Bilgi tabanı yükleme sırasında genel hata: " << e.what() << ". İçe aktarma başarısız.");
        }
    } else {
        LOG_DEFAULT(CerebrumLux::LogLevel::WARNING, "KnowledgeBase: JSON dosya acilamadi: " << file_path.string() << ". Dosya var ancak acilamiyor (izin veya kilit sorunu olabilir). İçe aktarma atlandı.");
    }
}

std::vector<Capsule> KnowledgeBase::get_all_capsules() const {
    LOG_DEFAULT(LogLevel::DEBUG, "KnowledgeBase::get_all_capsules(): Tüm aktif kapsüller LMDB'den isteniyor.");
    std::vector<Capsule> all_capsules;

    if (!m_swarm_db.is_open()) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "KnowledgeBase::get_all_capsules(): SwarmVectorDB acik degil. Kapsuller getirilemedi.");
        return {};
    }

    std::vector<std::string> all_ids = m_swarm_db.get_all_ids();

    // DÜZELTME: Tek tek okumak yerine toplu okuma (Batch Read) kullanılıyor
    auto cryptofig_vectors = m_swarm_db.get_vectors_batch(all_ids);

    for (const auto& cv : cryptofig_vectors) {
        if (cv) {
            all_capsules.push_back(convert_cryptofig_vector_to_capsule(*cv));
        }
    }

    LOG_DEFAULT(LogLevel::DEBUG, "KnowledgeBase::get_all_capsules(): Toplam " << all_capsules.size() << " kapsül LMDB'den alındı.");
    return all_capsules;
}

} // namespace CerebrumLux