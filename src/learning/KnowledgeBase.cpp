#include "KnowledgeBase.h"
#include <fstream>
#include <algorithm>
#include <numeric>
#include <cmath>
#include "../../core/logger.h"

namespace CerebrumLux { // Buradan itibaren CerebrumLux namespace'i başlar

KnowledgeBase::KnowledgeBase() {
    LOG_DEFAULT(LogLevel::INFO, "KnowledgeBase: Initialized.");
}

void KnowledgeBase::add_capsule(const Capsule& capsule) {
    auto it = std::find_if(capsules.begin(), capsules.end(),
                           [&](const Capsule& c){ return c.id == capsule.id; });

    if (it == capsules.end()) {
        capsules.push_back(capsule);
        LOG_DEFAULT(LogLevel::INFO, "KnowledgeBase: New capsule added. ID: " << capsule.id);
    } else {
        // Mevcut kapsülü güncelle
        *it = capsule;
        LOG_DEFAULT(LogLevel::INFO, "KnowledgeBase: Existing capsule updated. ID: " << capsule.id);
    }
}

std::vector<Capsule> KnowledgeBase::semantic_search(const std::string& query, int top_k) const {
    std::vector<Capsule> results;
    for (const auto& capsule : capsules) {
        if (capsule.content.find(query) != std::string::npos ||
            capsule.topic.find(query) != std::string::npos) {
            results.push_back(capsule);
        }
    }
    // TODO: Gerçek anlamsal arama algoritmaları burada uygulanacak.
    return results;
}

std::vector<Capsule> KnowledgeBase::search_by_topic(const std::string& topic) const {
    std::vector<Capsule> results;
    for (const auto& capsule : capsules) {
        if (capsule.topic == topic) {
            results.push_back(capsule);
        }
    }
    return results;
}

std::optional<Capsule> KnowledgeBase::find_capsule_by_id(const std::string& id) const {
    auto it = std::find_if(capsules.begin(), capsules.end(),
                           [&](const Capsule& c){ return c.id == id; });
    if (it != capsules.end()) {
        return *it;
    }
    return std::nullopt;
}

void KnowledgeBase::quarantine_capsule(const std::string& id) {
    auto it = std::remove_if(capsules.begin(), capsules.end(),
                             [&](const Capsule& c){ return c.id == id; });

    if (it != capsules.end()) {
        for (auto current = it; current != capsules.end(); ++current) {
            quarantined_capsules.push_back(std::move(*current));
        }
        capsules.erase(it, capsules.end());
        LOG_DEFAULT(LogLevel::WARNING, "KnowledgeBase: Capsule ID " << id << " quarantined.");
    } else {
        LOG_DEFAULT(LogLevel::INFO, "KnowledgeBase: Capsule ID " << id << " not found to quarantine.");
    }
}

void KnowledgeBase::revert_capsule(const std::string& id) {
    auto it = std::remove_if(quarantined_capsules.begin(), quarantined_capsules.end(),
                             [&](const Capsule& c){ return c.id == id; });

    if (it != quarantined_capsules.end()) {
        for (auto current = it; current != quarantined_capsules.end(); ++current) {
            capsules.push_back(std::move(*current));
        }
        quarantined_capsules.erase(it, quarantined_capsules.end());
        LOG_DEFAULT(LogLevel::INFO, "KnowledgeBase: Capsule ID " << id << " reverted from quarantine.");
    } else {
        LOG_DEFAULT(LogLevel::INFO, "KnowledgeBase: Capsule ID " << id << " not found in quarantine to revert.");
    }
}

void KnowledgeBase::save(const std::string& filename) const {
    std::ofstream ofs(filename);
    if (ofs.is_open()) {
        nlohmann::json j;
        for (const auto& c : capsules) {
            j["capsules"].push_back(c);
        }
        for (const auto& c : quarantined_capsules) {
            j["quarantined_capsules"].push_back(c);
        }
        ofs << j.dump(4);
        LOG_DEFAULT(LogLevel::INFO, "KnowledgeBase: Saved to " << filename << ".");
    } else {
        LOG_DEFAULT(LogLevel::ERR_CRITICAL, "KnowledgeBase: Could not open file for saving: " << filename);
    }
}

void KnowledgeBase::load(const std::string& filename) {
    std::ifstream ifs(filename);
    if (ifs.is_open()) {
        try {
            nlohmann::json j;
            ifs >> j;
            capsules.clear();
            quarantined_capsules.clear();
            if (j.contains("capsules")) {
                for (const auto& item : j["capsules"]) {
                    capsules.push_back(item.get<Capsule>());
                }
            }
            if (j.contains("quarantined_capsules")) {
                for (const auto& item : j["quarantined_capsules"]) {
                    quarantined_capsules.push_back(item.get<Capsule>());
                }
            }
            LOG_DEFAULT(LogLevel::INFO, "KnowledgeBase: Loaded from " << filename << ". " << capsules.size() << " active, " << quarantined_capsules.size() << " quarantined capsules.");
        } catch (const nlohmann::json::parse_error& e) {
            LOG_DEFAULT(LogLevel::ERR_CRITICAL, "KnowledgeBase: JSON parse error while loading " << filename << ": " << e.what());
        } catch (const std::exception& e) {
            LOG_DEFAULT(LogLevel::ERR_CRITICAL, "KnowledgeBase: Error while loading " << filename << ": " << e.what());
        }
    } else {
        LOG_DEFAULT(LogLevel::WARNING, "KnowledgeBase: Could not open file for loading: " << filename << ". Starting with empty knowledge base.");
    }
}

std::vector<float> KnowledgeBase::computeEmbedding(const std::string& text) const {
    std::hash<std::string> hasher;
    size_t hash_val = hasher(text);
    std::vector<float> embedding(32);
    for (size_t i = 0; i < 32; ++i) {
        embedding[i] = static_cast<float>((hash_val >> (i % 64)) & 0xFF) / 255.0f;
    }
    return embedding;
}

float KnowledgeBase::cosineSimilarity(const std::vector<float>& vec1, const std::vector<float>& vec2) const {
    if (vec1.empty() || vec1.size() != vec2.size()) {
        return 0.0f;
    }

    float dot_product = 0.0f;
    float norm1 = 0.0f;
    float norm2 = 0.0f;

    for (size_t i = 0; i < vec1.size(); ++i) {
        dot_product += vec1[i] * vec2[i];
        norm1 += vec1[i] * vec1[i];
        norm2 += vec2[i] * vec2[i];
    }

    if (norm1 == 0.0f || norm2 == 0.0f) {
        return 0.0f;
    }

    return dot_product / (std::sqrt(norm1) * std::sqrt(norm2));
}

} // namespace CerebrumLux