#ifndef AI_INSIGHTS_ENGINE_H
#define AI_INSIGHTS_ENGINE_H

#pragma once

#include <string>
#include <vector>
#include <map> // Cooldown mekanizması için
#include <chrono> // Cooldown mekanizması için

#include "../brain/intent_analyzer.h"
#include "../brain/intent_learner.h"
#include "../brain/prediction_engine.h"
#include "../brain/autoencoder.h" // CryptofigAutoencoder tanımı için
#include "../brain/cryptofig_processor.h"
#include "../data_models/dynamic_sequence.h"
#include "../core/enums.h" // InsightType, UrgencyLevel için (CerebrumLux namespace'i içinde)
#include "../core/logger.h" // LOG_DEFAULT için
#include "../core/utils.h" // SafeRNG için

#include "../external/nlohmann/json.hpp" // JSON için

namespace CerebrumLux { // AIInsight struct'ı ve AIInsightsEngine sınıfı bu namespace içine alınmalı

// AI tarafından üretilen tek bir içgörüyü temsil eder
struct AIInsight {
    std::string id;
    std::string observation; // Gözlem veya keşif
    std::string context;     // İçgörünün türetildiği bağlam
    std::string recommended_action; // Önerilen eylem
    InsightType type;        // İçgörünün türü (örneğin, "PerformanceAnomaly", "LearningOpportunity")
    UrgencyLevel urgency;    // İçgörünün aciliyet seviyesi
    std::vector<float> associated_cryptofig; // İlişkili kriptofig
    std::vector<std::string> related_capsule_ids;
    std::string code_file_path; // Aktif üye oldu!
    std::string source_cryptofig_id; // YENİ: Kaynak CryptofigVector'ün ID'si

    // Default constructor for aggregate initialization
    AIInsight() : id(""), observation(""), context(""), recommended_action(""),
                  type(InsightType::None), urgency(UrgencyLevel::None),
                  code_file_path("") {}

    // Complete constructor
    AIInsight(const std::string& id_val,
                       const std::string& observation_val,
                       const std::string& context_val,
                       const std::string& recommended_action_val,
                       InsightType type_val,
                       UrgencyLevel urgency_val,
                       const std::vector<float>& cryptofig_val = {},
                       const std::vector<std::string>& capsule_ids_val = {},
                       const std::string& code_file_path_val = "",
                       const std::string& source_cryptofig_id_val = "")
        : id(id_val),
          observation(observation_val),
          context(context_val),
          recommended_action(recommended_action_val),
          type(type_val),
          urgency(urgency_val),
          // associated_cryptofig'i her zaman INPUT_DIM boyutunda başlat
          associated_cryptofig(CerebrumLux::CryptofigAutoencoder::INPUT_DIM, 0.0f),
          related_capsule_ids(capsule_ids_val),
          code_file_path(code_file_path_val),
          source_cryptofig_id(source_cryptofig_id_val) {
            // Contradiction Resolution: Copy the contents from the passed vector.
            if (!cryptofig_val.empty()) {
                std::copy(cryptofig_val.begin(), 
                          cryptofig_val.begin() + std::min(cryptofig_val.size(), (size_t)CerebrumLux::CryptofigAutoencoder::INPUT_DIM), 
                          associated_cryptofig.begin());
            }
          }

    // Açıkça Copy/Move Semantics tanımları (varsayılanlar kullanılabilir)
    AIInsight(const AIInsight& other) = default; 
    AIInsight& operator=(const AIInsight& other) = default;
    AIInsight(AIInsight&& other) noexcept = default;
    AIInsight& operator=(AIInsight&& other) noexcept = default;
    
    // nlohmann::json ile serileştirme için friend fonksiyonlar
    friend void to_json(nlohmann::json& j, const AIInsight& i) {
        LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "AIInsight::to_json: Serileştirme basladi (ID: " << i.id << ", Type: " << static_cast<int>(i.type) << ")");
        
        std::vector<float> final_cryptofig = i.associated_cryptofig;
        if (final_cryptofig.size() != CerebrumLux::CryptofigAutoencoder::INPUT_DIM) {
            final_cryptofig.resize(CerebrumLux::CryptofigAutoencoder::INPUT_DIM, 0.0f);
            LOG_DEFAULT(CerebrumLux::LogLevel::WARNING, "AIInsight::to_json: associated_cryptofig boyutu INPUT_DIM ile uyuşmuyor. Boyut düzeltildi. ID: " << i.id);
        }

        j["id"] = i.id;
        j["observation"] = i.observation;
        j["context"] = i.context;
        j["recommended_action"] = i.recommended_action;
        j["type"] = static_cast<int>(i.type);
        j["urgency"] = static_cast<int>(i.urgency);
        j["associated_cryptofig"] = final_cryptofig;
        j["related_capsule_ids"] = i.related_capsule_ids;
        j["code_file_path"] = i.code_file_path;
        j["source_cryptofig_id"] = i.source_cryptofig_id;
    }

    friend void from_json(const nlohmann::json& j, AIInsight& i) {
        j.at("id").get_to(i.id);
        j.at("observation").get_to(i.observation);
        j.at("context").get_to(i.context);
        j.at("recommended_action").get_to(i.recommended_action);
        i.type = static_cast<InsightType>(j.at("type").get<int>());
        i.urgency = static_cast<UrgencyLevel>(j.at("urgency").get<int>());

        if (j.contains("associated_cryptofig") && j.at("associated_cryptofig").is_array()) {
            j.at("associated_cryptofig").get_to(i.associated_cryptofig);
            if (i.associated_cryptofig.size() != CerebrumLux::CryptofigAutoencoder::INPUT_DIM) {
                i.associated_cryptofig.resize(CerebrumLux::CryptofigAutoencoder::INPUT_DIM, 0.0f);
            }
        } else {
            i.associated_cryptofig.assign(CerebrumLux::CryptofigAutoencoder::INPUT_DIM, 0.0f);
        }

        j.at("related_capsule_ids").get_to(i.related_capsule_ids);
        j.at("code_file_path").get_to(i.code_file_path);
        if (j.contains("source_cryptofig_id")) j.at("source_cryptofig_id").get_to(i.source_cryptofig_id);
    }
};

// AI Insights Engine sınıfı
class AIInsightsEngine {
public:
    AIInsightsEngine(IntentAnalyzer& analyzer, IntentLearner& learner, PredictionEngine& predictor,
                     CryptofigAutoencoder& autoencoder, CryptofigProcessor& cryptofig_processor);


    // ABI uyumluluğu ve modül bağımsızlığı için JSON string'i döndürür.
    std::string generate_insights(const DynamicSequence& current_sequence);
    
    float calculate_autoencoder_reconstruction_error(const std::vector<float>& statistical_features) const;

    virtual IntentAnalyzer& get_analyzer() const;

    virtual IntentLearner& get_learner() const { return intent_learner; }
    virtual CryptofigAutoencoder& get_cryptofig_autoencoder() const { return cryptofig_autoencoder; }

private:
    IntentAnalyzer& intent_analyzer;
    IntentLearner& intent_learner;
    PredictionEngine& prediction_engine;
    CryptofigAutoencoder& cryptofig_autoencoder;
    CryptofigProcessor& cryptofig_processor;

    std::map<std::string, int> analyzed_file_loc_metrics; // CodeAnalyzerUtils için
    std::vector<std::string> files_to_analyze; // CodeAnalyzerUtils için

    // --- Dahili Durum ve Yardımcı Metotlar ---
    std::map<std::string, std::chrono::system_clock::time_point> insight_cooldowns; // İçgörü türleri için bekleme süreleri (tek tanım)
    bool is_on_cooldown(const std::string& key, std::chrono::seconds cooldown_duration) const; // Yardımcı metot deklarasyonu

    // Refaktör sonrası yeni üye metotları deklarasyonları
    void updateSimulatedCodeMetrics(std::chrono::system_clock::time_point now);
    std::vector<AIInsight> generateCodeAnalysisInsights(const DynamicSequence& current_sequence, std::chrono::system_clock::time_point now);
    std::vector<AIInsight> generateSimulatedMetricCodeDevelopmentInsights(const DynamicSequence& current_sequence, std::chrono::system_clock::time_point now);
    std::vector<AIInsight> generateGeneralNonCodeDevelopmentInsights(const DynamicSequence& current_sequence, std::chrono::system_clock::time_point now);

    // Mevcut yardımcı içgörü üretim metotları
    AIInsight generate_reconstruction_error_insight(const DynamicSequence& current_sequence);
    AIInsight generate_learning_rate_insight(const DynamicSequence& current_sequence);
    AIInsight generate_system_resource_insight(const DynamicSequence& current_sequence);
    AIInsight generate_network_activity_insight(const DynamicSequence& current_sequence);
    AIInsight generate_application_context_insight(const DynamicSequence& current_sequence);
    AIInsight generate_unusual_behavior_insight(const DynamicSequence& current_sequence);

    // Simüle edilmiş kod metrikleri
    float last_simulated_code_complexity = 0.95f;
    float last_simulated_code_readability = 0.25f;
    float last_simulated_optimization_potential = 0.85f;
};

} // namespace CerebrumLux

#endif // AI_INSIGHTS_ENGINE_H