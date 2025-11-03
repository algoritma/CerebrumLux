#ifndef KNOWLEDGEBASE_H
#define KNOWLEDGEBASE_H

#include <string>
#include <vector> // Sadece get_all_capsules için return type olarak kalabilir
#include <optional> // std::optional için
#include <nlohmann/json.hpp>
#include "Capsule.h" // CerebrumLux::Capsule struct'ı için

#include "../swarm_vectordb/VectorDB.h" // SwarmVectorDB entegrasyonu için
#include "../swarm_vectordb/DataModels.h" // CryptofigVector için
#include <Eigen/Dense> // Embedding dönüşümü için

namespace CerebrumLux { // Buradan itibaren CerebrumLux namespace'i başlar

class KnowledgeBase {
public:
    KnowledgeBase(); // Varsayılan kurucu
    KnowledgeBase(const std::string& db_path); // LMDB yolu ile yeni kurucu
    void add_capsule(const Capsule& capsule); 
    std::vector<Capsule> semantic_search(const std::vector<float>& query_embedding, int top_k = 3) const;
    std::vector<Capsule> search_by_topic(const std::vector<float>& topic_embedding, int top_k = 3) const;
    std::optional<Capsule> find_capsule_by_id(const std::string& id) const; // const eklendi
    void quarantine_capsule(const std::string& id);
    void revert_capsule(const std::string& id); // Karantinadan geri alma

    // JSON'a dışa aktırma ve JSON'dan içe aktarma (ana operasyonlar için değil, araçlar için)
    void export_to_json(const std::string& filename = "knowledge_export.json") const;
    void import_from_json(const std::string& filename = "knowledge_import.json"); // Adı değiştirildi
   
    // LMDB tabanlı SwarmVectorDB'ye doğrudan erişim için getter'lar
    CerebrumLux::SwarmVectorDB::SwarmVectorDB& get_swarm_db() { return swarm_db_; }
    const CerebrumLux::SwarmVectorDB::SwarmVectorDB& get_swarm_db() const { return swarm_db_; }
    virtual std::vector<Capsule> get_all_capsules() const; // Reverted to const

    float cosineSimilarity(const std::vector<float>& vec1, const std::vector<float>& vec2) const; // Moved from public to private/protected if not used externally


private:
    std::string db_path_; // LMDB veritabanı yolu
    mutable CerebrumLux::SwarmVectorDB::SwarmVectorDB swarm_db_; // LMDB tabanlı veritabanı

    // Yardımcı dönüşüm metodları
    SwarmVectorDB::CryptofigVector convert_capsule_to_cryptofig_vector(const Capsule& capsule) const;
    Capsule convert_cryptofig_vector_to_capsule(const SwarmVectorDB::CryptofigVector& cv) const;

    // Eski in-memory kapsül listeleri kaldırıldı, artık LMDB kullanılıyor.
    // std::vector<Capsule> capsules;
    // std::vector<Capsule> quarantined_capsules;
};

} // namespace CerebrumLux // Namespace burada biter

#endif // KNOWLEDGEBASE_H