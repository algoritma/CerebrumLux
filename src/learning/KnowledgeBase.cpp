#include "KnowledgeBase.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include <cmath>
#include <algorithm> // std::sort için

// YENİ: Varsayılan bilgi tabanı dosyası adı
const std::string DEFAULT_KNOWLEDGE_BASE_FILE = "knowledge_base.json"; 


// Basit fake embedding hesaplayıcı (gerçek model entegre edilebilir)
std::vector<float> KnowledgeBase::computeEmbedding(const std::string& text) {
    std::vector<float> emb(16, 0.0f); // 16 boyutlu küçük vektör
    for (size_t i = 0; i < text.size(); i++) {
        emb[i % 16] += static_cast<float>(text[i]) / 255.0f;
    }
    return emb;
}

float KnowledgeBase::cosineSimilarity(const std::vector<float>& a, const std::vector<float>& b) {
    float dot = 0, normA = 0, normB = 0;
    for (size_t i = 0; i < a.size(); i++) {
        dot += a[i] * b[i];
        normA += a[i] * a[i];
        normB += b[i] * b[i];
    }
    return dot / (std::sqrt(normA) * std::sqrt(normB) + 1e-6);
}

void KnowledgeBase::addCapsule(const Capsule& capsule) {
    Capsule c = capsule;
    c.embedding = computeEmbedding(c.content);
    capsules.push_back(c);
}

std::vector<Capsule> KnowledgeBase::findSimilar(const std::string& query, int top_k) {
    std::vector<float> q_emb = computeEmbedding(query);
    std::vector<std::pair<float, Capsule>> scored;

    for (const auto& c : capsules) {
        float sim = cosineSimilarity(q_emb, c.embedding);
        scored.push_back({sim, c});
    }

    std::sort(scored.begin(), scored.end(), [](auto& a, auto& b) {
        return a.first > b.first;
    });

    std::vector<Capsule> results;
    for (int i = 0; i < top_k && i < (int)scored.size(); i++) {
        results.push_back(scored[i].second);
    }
    return results;
}

// YENİ: Belirli bir konuya göre kapsülleri döndürür
std::vector<Capsule> KnowledgeBase::getCapsulesByTopic(const std::string& topic) const {
    std::vector<Capsule> filtered_capsules;
    // Basit bir konu eşleşmesi yapalım, daha sonra daha gelişmiş bir mekanizma eklenebilir.
    // Örneğin, konu etiketleri veya içerik analizi kullanılabilir.
    for (const auto& capsule : capsules) {
        if (capsule.topic == topic) { // Capsule struct'ında 'topic' üyesi olduğunu varsayıyoruz
            filtered_capsules.push_back(capsule);
        }
    }
    return filtered_capsules;
}

// YENİ: Metin şifreleme metodu implementasyonu (basit XOR)
std::string KnowledgeBase::encrypt(const std::string& data) const {
    std::string encrypted_data = data;
    char key = 0x5A; // Basit bir XOR anahtarı (01011010)
    for (char &c : encrypted_data) {
        c ^= key;
    }
    return encrypted_data;
}

// YENİ: Parametresiz save metodu implementasyonu
// Varsayılan bir dosya adını kullanır.
void KnowledgeBase::save() const {
    save(DEFAULT_KNOWLEDGE_BASE_FILE); // Diğer save metodunu çağırır
}


void KnowledgeBase::save(const std::string& filename) const { // DÜZELTİLDİ: const eklendi
    nlohmann::json j;
    for (const auto& c : capsules) {
        j.push_back(c.toJson());
    }
    std::ofstream f(filename);
    f << j.dump(2);
}

void KnowledgeBase::load(const std::string& filename) {
    std::ifstream f(filename);
    if (!f.is_open()) return;
    nlohmann::json j;
    f >> j;
    capsules.clear();
    for (auto& el : j) {
        capsules.push_back(Capsule::fromJson(el));
    }
}