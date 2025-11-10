#ifndef KNOWLEDGEBASE_H
#define KNOWLEDGEBASE_H

#include <string>
#include <vector>
#include <optional>     // std::optional için
#include <nlohmann/json.hpp> // JSON serileştirme için
#include "Capsule.h"    // CerebrumLux::Capsule struct'ı için

// SwarmVectorDB entegrasyonu için gerekli başlıklar
#include "../swarm_vectordb/VectorDB.h"    // CerebrumLux::SwarmVectorDB::SwarmVectorDB sınıfı için
#include "../swarm_vectordb/DataModels.h" // CryptofigVector ve EmbeddingStateKey için
#include <Eigen/Dense>  // Embedding dönüşümü için (CryptofigVector içinde kullanılıyor olabilir)

namespace CerebrumLux {

class KnowledgeBase {
public:
    // Kurucular ve Yıkıcı
    KnowledgeBase();                      // Varsayılan kurucu
    KnowledgeBase(const std::string& db_path); // LMDB yolu ile parametreli kurucu
    ~KnowledgeBase();                     // Yıkıcı

    // SwarmVectorDB'ye doğrudan erişim için getter'lar (const ve non-const versiyonları)
    // DÜZELTİLDİ: Dönüş tipi CerebrumLux::SwarmVectorDB::SwarmVectorDB olarak değiştirildi.
    CerebrumLux::SwarmVectorDB::SwarmVectorDB& get_swarm_db() { return m_swarm_db; }
    const CerebrumLux::SwarmVectorDB::SwarmVectorDB& get_swarm_db() const { return m_swarm_db; }

    // Kapsül Yönetim Metodları
    void add_capsule(const Capsule& capsule); 
    std::optional<Capsule> find_capsule_by_id(const std::string& id) const;
    void quarantine_capsule(const std::string& id);
    void revert_capsule(const std::string& id);

    // Arama ve İçerik Metodları
    std::vector<Capsule> semantic_search(const std::vector<float>& query_embedding, int top_k = 3) const;
    std::vector<Capsule> search_by_topic(const std::vector<float>& topic_embedding, int top_k = 3) const;
    virtual std::vector<Capsule> get_all_capsules() const;

    // JSON İçe/Dışa Aktarma Metodları (Araçlar için)
    void export_to_json(const std::string& filename = "knowledge_export.json") const;
    void import_from_json(const std::string& filename = "knowledge_import.json");

private:
    std::string db_path_; // LMDB veritabanı yolu
    // DÜZELTİLDİ: Üye değişken tipi CerebrumLux::SwarmVectorDB::SwarmVectorDB olarak değiştirildi.
    CerebrumLux::SwarmVectorDB::SwarmVectorDB m_swarm_db; // LMDB tabanlı vektör veritabanı üyesi

    // Yardımcı Dönüşüm Metodları
    SwarmVectorDB::CryptofigVector convert_capsule_to_cryptofig_vector(const Capsule& capsule) const;
    Capsule convert_cryptofig_vector_to_capsule(const SwarmVectorDB::CryptofigVector& cv) const;
};

} // namespace CerebrumLux

#endif // KNOWLEDGEBASE_H