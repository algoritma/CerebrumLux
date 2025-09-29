#ifndef KNOWLEDGEBASE_H
#define KNOWLEDGEBASE_H

#include <string>
#include <vector>
#include <optional> // std::optional için
#include <nlohmann/json.hpp>
#include "Capsule.h" // CerebrumLux::Capsule struct'ı için

namespace CerebrumLux { // Buradan itibaren CerebrumLux namespace'i başlar

class KnowledgeBase {
public:
    KnowledgeBase(); // Kurucu eklendi
    void add_capsule(const Capsule& capsule);
    std::vector<Capsule> semantic_search(const std::string& query, int top_k = 3) const;
    std::vector<Capsule> search_by_topic(const std::string& topic) const;
    std::optional<Capsule> find_capsule_by_id(const std::string& id) const;
    void quarantine_capsule(const std::string& id);
    void revert_capsule(const std::string& id);
    void save(const std::string& filename = "knowledge.json") const;
    void load(const std::string& filename = "knowledge.json");
    std::vector<float> computeEmbedding(const std::string& text) const;
    float cosineSimilarity(const std::vector<float>& vec1, const std::vector<float>& vec2) const;

    virtual std::vector<Capsule> get_all_capsules() const; // YENİ: Tüm kapsülleri döndüren metot (virtual yapıldı)

private:
    std::vector<Capsule> capsules;
    std::vector<Capsule> quarantined_capsules; // Karantinaya alınan kapsüller
};

} // namespace CerebrumLux // Namespace burada biter

#endif // KNOWLEDGEBASE_H