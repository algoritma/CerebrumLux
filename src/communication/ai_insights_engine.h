#ifndef AI_INSIGHTS_ENGINE_H
#define AI_INSIGHTS_ENGINE_H

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

    // NLOHMANN_DEFINE_TYPE_INTRUSIVE makrosu yerine manuel to_json ve from_json
    friend void to_json(nlohmann::json& j, const AIInsight& i) {
        j["id"] = i.id;
        j["observation"] = i.observation;
        j["context"] = i.context;
        j["recommended_action"] = i.recommended_action;
        j["type"] = static_cast<int>(i.type); // Enum'ları int olarak serileştir
        j["urgency"] = static_cast<int>(i.urgency); // Enum'ları int olarak serileştir
        j["associated_cryptofig"] = i.associated_cryptofig;
        j["related_capsule_ids"] = i.related_capsule_ids;
    }

    friend void from_json(const nlohmann::json& j, AIInsight& i) {
        j.at("id").get_to(i.id);
        j.at("observation").get_to(i.observation);
        j.at("context").get_to(i.context);
        j.at("recommended_action").get_to(i.recommended_action);
        i.type = static_cast<InsightType>(j.at("type").get<int>()); // int'ten enum'a dönüştür
        i.urgency = static_cast<UrgencyLevel>(j.at("urgency").get<int>()); // int'ten enum'a dönüştür
        j.at("associated_cryptofig").get_to(i.associated_cryptofig);
        j.at("related_capsule_ids").get_to(i.related_capsule_ids);
    }
};

// AI Insights Engine sınıfı
class AIInsightsEngine {
public:
    AIInsightsEngine(IntentAnalyzer& analyzer, IntentLearner& learner, PredictionEngine& predictor,
                     CryptofigAutoencoder& autoencoder, CryptofigProcessor& cryptofig_processor);

    std::vector<AIInsight> generate_insights(const DynamicSequence& current_sequence);
    
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

    std::map<std::string, std::chrono::system_clock::time_point> insight_cooldowns;
    bool is_on_cooldown(const std::string& key, std::chrono::seconds cooldown_duration) const;

    // YENİ YARDIMCI İÇGÖRÜ ÜRETİM METOTLARI DEKLARASYONLARI
    AIInsight generate_reconstruction_error_insight(const DynamicSequence& current_sequence);
    AIInsight generate_learning_rate_insight(const DynamicSequence& current_sequence);
    AIInsight generate_system_resource_insight(const DynamicSequence& current_sequence);
    AIInsight generate_network_activity_insight(const DynamicSequence& current_sequence);
    AIInsight generate_application_context_insight(const DynamicSequence& current_sequence);
    AIInsight generate_unusual_behavior_insight(const DynamicSequence& current_sequence); // Daha sonra geliştirilebilir
};

} // namespace CerebrumLux

#endif // AI_INSIGHTS_ENGINE_H