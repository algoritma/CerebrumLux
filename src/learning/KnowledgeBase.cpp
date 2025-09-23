#include "KnowledgeBase.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include <cmath>
#include <algorithm> // std::sort, std::remove_if için
#include <chrono>    // Kapsül zaman damgası için (toJson/fromJson'da kullanılıyor, burada dolaylı)
#include "../core/logger.h" // LOG_DEFAULT için
#include "../core/utils.h" // hash_string gibi yardımcılar için (gerekliyse)


// YENİ: Varsayılan bilgi tabanı dosyası adı
const std::string DEFAULT_KNOWLEDGE_BASE_FILE = "knowledge_base.json"; 
const std::string QUARANTINE_KNOWLEDGE_BASE_FILE = "quarantine_knowledge_base.json"; // YENİ: Karantina dosyası

// Basit fake embedding hesaplayıcı (gerçek model entegre edilebilir)
std::vector<float> KnowledgeBase::computeEmbedding(const std::string& text) const {
    std::vector<float> emb(16, 0.0f); // 16 boyutlu küçük vektör
    for (size_t i = 0; i < text.size(); i++) {
        emb[i % 16] += static_cast<float>(text[i]) / 255.0f;
    }
    return emb;
}

float KnowledgeBase::cosineSimilarity(const std::vector<float>& a, const std::vector<float>& b) const {
    float dot = 0, normA = 0, normB = 0;
    if (a.empty() || b.empty() || a.size() != b.size()) {
        return 0.0f; // Boyut uyuşmazlığı durumunda 0 döndür
    }
    for (size_t i = 0; i < a.size(); i++) {
        dot += a[i] * b[i];
        normA += a[i] * a[i];
        normB += b[i] * b[i];
    }
    float denominator = std::sqrt(normA) * std::sqrt(normB);
    if (denominator < 1e-6) return 0.0f; // Sıfıra bölme hatasını önle
    return dot / denominator;
}

// addCapsule -> add_capsule
void KnowledgeBase::add_capsule(const Capsule& capsule) {
    Capsule c = capsule;
    c.embedding = computeEmbedding(c.content);
    capsules.push_back(c);
    LOG_DEFAULT(LogLevel::INFO, "KnowledgeBase: Kapsül eklendi. ID: " << c.id << ", Konu: " << c.topic);
}

// findSimilar -> semantic_search
std::vector<Capsule> KnowledgeBase::semantic_search(const std::string& query, int top_k) const { // CONST EKLENDİ
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
    LOG_DEFAULT(LogLevel::DEBUG, "KnowledgeBase: Semantik arama tamamlandı. Sorgu: '" << query << "', Sonuç sayısı: " << results.size());
    return results;
}

// getCapsulesByTopic -> search_by_topic
std::vector<Capsule> KnowledgeBase::search_by_topic(const std::string& topic) const {
    std::vector<Capsule> filtered_capsules;
    for (const auto& capsule : capsules) {
        if (capsule.topic == topic) {
            filtered_capsules.push_back(capsule);
        }
    }
    LOG_DEFAULT(LogLevel::DEBUG, "KnowledgeBase: Konuya göre arama tamamlandı. Konu: '" << topic << "', Sonuç sayısı: " << filtered_capsules.size());
    return filtered_capsules;
}

// YENİ: Capsule ID'ye göre arama metodu eklendi
std::optional<Capsule> KnowledgeBase::find_capsule_by_id(const std::string& id) const {
    for (const auto& capsule : capsules) {
        if (capsule.id == id) {
            return capsule;
        }
    }
    for (const auto& capsule : quarantined_capsules) { // Karantinadaki kapsüller de kontrol edilir
        if (capsule.id == id) {
            return capsule;
        }
    }
    LOG_DEFAULT(LogLevel::WARNING, "KnowledgeBase: ID ile kapsül bulunamadı: " << id);
    return std::nullopt;
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

// YENİ: Metin şifre çözme metodu implementasyonu (basit XOR)
std::string KnowledgeBase::decrypt(const std::string& encrypted_data) const {
    // XOR işlemi kendi tersidir, bu yüzden aynı fonksiyonu kullanabiliriz
    return encrypt(encrypted_data);
}

// YENİ: Kapsülü karantinaya al
bool KnowledgeBase::quarantine_capsule(const std::string& capsule_id) {
    auto it = std::remove_if(capsules.begin(), capsules.end(), [&](const Capsule& c){
        return c.id == capsule_id;
    });

    if (it != capsules.end()) {
        quarantined_capsules.insert(quarantined_capsules.end(), std::make_move_iterator(it), std::make_move_iterator(capsules.end()));
        capsules.erase(it, capsules.end());
        LOG_DEFAULT(LogLevel::WARNING, "KnowledgeBase: Kapsül karantinaya alındı. ID: " << capsule_id);
        save(DEFAULT_KNOWLEDGE_BASE_FILE);
        save_quarantined_capsules(); // Karantina kapsüllerini de kaydet
        return true;
    }
    LOG_DEFAULT(LogLevel::WARNING, "KnowledgeBase: Karantinaya alınacak kapsül bulunamadı. ID: " << capsule_id);
    return false;
}

// YENİ: Karantinadaki kapsülü geri al
bool KnowledgeBase::revert_capsule(const std::string& capsule_id) {
    auto it = std::remove_if(quarantined_capsules.begin(), quarantined_capsules.end(), [&](const Capsule& c){
        return c.id == capsule_id;
    });

    if (it != quarantined_capsules.end()) {
        capsules.insert(capsules.end(), std::make_move_iterator(it), std::make_move_iterator(quarantined_capsules.end()));
        quarantined_capsules.erase(it, quarantined_capsules.end());
        LOG_DEFAULT(LogLevel::INFO, "KnowledgeBase: Karantinadaki kapsül geri alındı. ID: " << capsule_id);
        save(DEFAULT_KNOWLEDGE_BASE_FILE);
        save_quarantined_capsules(); // Karantina kapsüllerini de kaydet
        return true;
    }
    LOG_DEFAULT(LogLevel::WARNING, "KnowledgeBase: Geri alınacak kapsül karantinada bulunamadı. ID: " << capsule_id);
    return false;
}

// YENİ: Karantinadaki kapsülleri kaydetme
void KnowledgeBase::save_quarantined_capsules() const {
    nlohmann::json j;
    for (const auto& c : quarantined_capsules) {
        j.push_back(c.toJson());
    }
    std::ofstream f(QUARANTINE_KNOWLEDGE_BASE_FILE);
    if (f.is_open()) {
        f << j.dump(2);
        LOG_DEFAULT(LogLevel::INFO, "KnowledgeBase: Karantinadaki kapsüller kaydedildi: " << QUARANTINE_KNOWLEDGE_BASE_FILE);
    } else {
        LOG_DEFAULT(LogLevel::ERR_CRITICAL, "KnowledgeBase: Karantina kapsül dosyası açılamadı: " << QUARANTINE_KNOWLEDGE_BASE_FILE);
    }
}

// YENİ: Karantinadaki kapsülleri yükleme
void KnowledgeBase::load_quarantined_capsules() {
    std::ifstream f(QUARANTINE_KNOWLEDGE_BASE_FILE);
    if (!f.is_open()) {
        LOG_DEFAULT(LogLevel::WARNING, "KnowledgeBase: Karantinadaki kapsül dosyası bulunamadı, boş olarak başlatılıyor: " << QUARANTINE_KNOWLEDGE_BASE_FILE);
        return;
    }
    nlohmann::json j;
    try {
        f >> j;
        quarantined_capsules.clear();
        for (auto& el : j) {
            quarantined_capsules.push_back(Capsule::fromJson(el));
        }
        LOG_DEFAULT(LogLevel::INFO, "KnowledgeBase: Karantinadaki kapsüller yüklendi: " << QUARANTINE_KNOWLEDGE_BASE_FILE << ", Sayı: " << quarantined_capsules.size());
    } catch (const nlohmann::json::parse_error& e) {
        LOG_DEFAULT(LogLevel::ERR_CRITICAL, "KnowledgeBase: Karantina JSON dosyası ayrıştırma hatası: " << e.what());
    } catch (const std::exception& e) {
        LOG_DEFAULT(LogLevel::ERR_CRITICAL, "KnowledgeBase: Karantina kapsüllerini yüklerken hata oluştu: " << e.what());
    }
}

// Parametresiz save metodu implementasyonu
void KnowledgeBase::save() const {
    save(DEFAULT_KNOWLEDGE_BASE_FILE); // Diğer save metodunu çağırır
    save_quarantined_capsules(); // Karantinadaki kapsülleri de kaydet
}

void KnowledgeBase::save(const std::string& filename) const {
    nlohmann::json j;
    for (const auto& c : capsules) {
        j.push_back(c.toJson());
    }
    std::ofstream f(filename);
    if (f.is_open()) {
        f << j.dump(2);
        LOG_DEFAULT(LogLevel::INFO, "KnowledgeBase: Bilgi tabanı kaydedildi: " << filename);
    } else {
        LOG_DEFAULT(LogLevel::ERR_CRITICAL, "KnowledgeBase: Bilgi tabanı dosyası açılamadı: " << filename);
    }
}

void KnowledgeBase::load(const std::string& filename) {
    std::ifstream f(filename);
    if (!f.is_open()) {
        LOG_DEFAULT(LogLevel::WARNING, "KnowledgeBase: Bilgi tabanı dosyası bulunamadı, boş olarak başlatılıyor: " << filename);
        return;
    }
    nlohmann::json j;
    try {
        f >> j;
        capsules.clear();
        for (auto& el : j) {
            capsules.push_back(Capsule::fromJson(el));
        }
        LOG_DEFAULT(LogLevel::INFO, "KnowledgeBase: Bilgi tabanı yüklendi: " << filename << ", Sayı: " << capsules.size());
    } catch (const nlohmann::json::parse_error& e) {
        LOG_DEFAULT(LogLevel::ERR_CRITICAL, "KnowledgeBase: JSON dosyası ayrıştırma hatası: " << e.what());
    } catch (const std::exception& e) {
        LOG_DEFAULT(LogLevel::ERR_CRITICAL, "KnowledgeBase: Kapsülleri yüklerken hata oluştu: " << e.what());
    }
    load_quarantined_capsules(); // Karantinadaki kapsülleri de yükle
}