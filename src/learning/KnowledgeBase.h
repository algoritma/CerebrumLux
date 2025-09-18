#pragma once
#include "Capsule.h"
#include <vector>
#include <string>

class KnowledgeBase {
public:
    void addCapsule(const Capsule& capsule);
    std::vector<Capsule> findSimilar(const std::string& query, int top_k = 3);

    void save(const std::string& filename) const; // DÜZELTİLDİ: const eklendi
    void load(const std::string& filename);

    // YENİ: LearningModule tarafından çağrılan metodun bildirimi
    std::vector<Capsule> getCapsulesByTopic(const std::string& topic) const;

    // YENİ: Metin şifreleme metodu
    std::string encrypt(const std::string& data) const;

    // YENİ: Parametresiz save metodu (varsayılan dosya adıyla kaydetmek için)
    void save() const; 

private:
    std::vector<Capsule> capsules;

    std::vector<float> computeEmbedding(const std::string& text);
    float cosineSimilarity(const std::vector<float>& a, const std::vector<float>& b);
};