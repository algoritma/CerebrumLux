#pragma once
#include "Capsule.h"
#include <vector>
#include <string>
#include <optional> // YENİ: find_capsule_by_id için

class KnowledgeBase {
public:
    // Metot isimlendirmeleri güncellendi
    void add_capsule(const Capsule& capsule); // addCapsule -> add_capsule
    std::vector<Capsule> semantic_search(const std::string& query, int top_k = 3) const; // findSimilar -> semantic_search, const eklendi
    std::vector<Capsule> search_by_topic(const std::string& topic) const; // getCapsulesByTopic -> search_by_topic
    
    // YENİ: Capsule ID'ye göre arama metodu eklendi
    std::optional<Capsule> find_capsule_by_id(const std::string& id) const;

    // Karantina ve geri alma metotları eklendi
    bool quarantine_capsule(const std::string& capsule_id);
    bool revert_capsule(const std::string& capsule_id);

    // Kaydetme/Yükleme metotları
    void save(const std::string& filename) const;
    void load(const std::string& filename);
    void save() const; // Parametresiz save metodu

    // Şifreleme metodu (şimdilik basit XOR, LearningModule'de güçlendirilecek)
    std::string encrypt(const std::string& data) const;
    std::string decrypt(const std::string& encrypted_data) const; // YENİ: Decrypt metodu eklendi

    // Embedding hesaplama metodunu public ve CONST yapıyoruz
    std::vector<float> computeEmbedding(const std::string& text) const; // CONST EKLENDİ
    float cosineSimilarity(const std::vector<float>& a, const std::vector<float>& b) const; // CONST EKLENDİ

    // Karantinadaki kapsülleri kaydetme/yükleme bildirimleri
    void save_quarantined_capsules() const; 
    void load_quarantined_capsules();     

private:
    std::vector<Capsule> capsules;
    std::vector<Capsule> quarantined_capsules; // Karantinaya alınan kapsüller
};