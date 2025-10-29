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
    std::vector<Capsule> semantic_search(const std::string& query, int top_k = 3) const;
    std::vector<Capsule> search_by_topic(const std::string& topic) const;
    std::optional<Capsule> find_capsule_by_id(const std::string& id); 
    void quarantine_capsule(const std::string& id);
    void revert_capsule(const std::string& id);
    void save(const std::string& filename = "knowledge.json") const;
    void load(const std::string& filename = "knowledge.json");
   
    // Bu metodlar LMDB entegrasyonu ile değişecek veya daha sonra refaktör edilecek
    std::vector<float> computeEmbedding(const std::string& text) const; 
    float cosineSimilarity(const std::vector<float>& vec1, const std::vector<float>& vec2) const; 

    virtual std::vector<Capsule> get_all_capsules() const; // YENİ: Tüm kapsülleri döndüren metot (virtual yapıldı)

private:
    std::string db_path_; // LMDB veritabanı yolu
    CerebrumLux::SwarmVectorDB::SwarmVectorDB swarm_db_; // LMDB tabanlı veritabanı

    // Yardımcı dönüşüm metodları
    SwarmVectorDB::CryptofigVector convert_capsule_to_cryptofig_vector(const Capsule& capsule) const;
    Capsule convert_cryptofig_vector_to_capsule(const SwarmVectorDB::CryptofigVector& cv) const;

    // Eski in-memory kapsül listeleri kaldırıldı, artık LMDB kullanılıyor.
    // std::vector<Capsule> capsules;
    // std::vector<Capsule> quarantined_capsules;
};

} // namespace CerebrumLux // Namespace burada biter

#endif // KNOWLEDGEBASE_H