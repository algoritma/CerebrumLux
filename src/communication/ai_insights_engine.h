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
    std::string code_file_path; // Aktif üye oldu!

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
                       const std::string& code_file_path_val = "")
        : id(id_val),
          observation(observation_val),
          context(context_val),
          recommended_action(recommended_action_val),
          type(type_val),
          urgency(urgency_val),
          associated_cryptofig(cryptofig_val),
          related_capsule_ids(capsule_ids_val),
          code_file_path(code_file_path_val) {}

    // YENİ EKLENDİ: Açıkça Copy/Move Semantics tanımları (varsayılanlar kullanılabilir)
    // Bunlar, nlohmann::json ile serileştirme/deserileştirme ve std::vector kopyalama/taşıma için önemlidir.
    AIInsight(const AIInsight& other) = default; // Varsayılan kopyalama constructor'ı
    AIInsight& operator=(const AIInsight& other) = default; // Varsayılan kopyalama atama operator'ü
    AIInsight(AIInsight&& other) noexcept = default; // Varsayılan taşıma constructor'ı
    AIInsight& operator=(AIInsight&& other) noexcept = default; // Varsayılan taşıma atama operator'ü
    
    // NLOHMANN_DEFINE_TYPE_INTRUSIVE makrosu yerine manuel to_json ve from_json
    friend void to_json(nlohmann::json& j, const AIInsight& i) {
        // ✅ Yeni Teşhis Logları: Serileştirme adımlarını izlemek için
        LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "AIInsight::to_json: Serileştirme basladi (ID: " << i.id << ", Type: " << static_cast<int>(i.type) << ")");
        j["id"] = i.id;
        j["observation"] = i.observation;
        j["context"] = i.context;
        j["recommended_action"] = i.recommended_action;
        j["type"] = static_cast<int>(i.type); // Enum'ları int olarak serileştir
        j["urgency"] = static_cast<int>(i.urgency); // Enum'ları int olarak serileştir
        j["associated_cryptofig"] = i.associated_cryptofig;
        j["related_capsule_ids"] = i.related_capsule_ids;
        j["code_file_path"] = i.code_file_path; // code_file_path da JSON'a eklendi
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
        //if (j.contains("code_file_path")) j.at("code_file_path").get_to(i.code_file_path); // Eğer varsa oku
        j.at("code_file_path").get_to(i.code_file_path); // Düzeltme: code_file_path'in her zaman var olduğunu varsayıyoruz (yoksa exception fırlatır)
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

    // YENİ YARDIMCI İÇGÖRÜ ÜRETİM METOTLARI DEKLARASYONLARI (code_file_path destekli)
    AIInsight generate_reconstruction_error_insight(const DynamicSequence& current_sequence);
    AIInsight generate_learning_rate_insight(const DynamicSequence& current_sequence);
    AIInsight generate_system_resource_insight(const DynamicSequence& current_sequence);
    AIInsight generate_network_activity_insight(const DynamicSequence& current_sequence);
    AIInsight generate_application_context_insight(const DynamicSequence& current_sequence);
    AIInsight generate_unusual_behavior_insight(const DynamicSequence& current_sequence); // Daha sonra geliştirilebilir

    // Simüle edilmiş kod metrikleri
    float last_simulated_code_complexity = 0.95f;
    float last_simulated_code_readability = 0.25f;
    float last_simulated_optimization_potential = 0.85f;
};

} // namespace CerebrumLux


// ==========================================================
// 🔧 JSON serileştirme erişimi için ADL düzeltmesi
// ==========================================================
// Bu blok, CerebrumLux namespace'indeki AIInsight struct'ının
// nlohmann::json ile doğru şekilde serileştirilmesini sağlar.
/*
namespace nlohmann {
    template <>
    struct adl_serializer<CerebrumLux::AIInsight> {
        static void to_json(json& j, const CerebrumLux::AIInsight& i) {
            j = json{
                {"id", i.id},
                {"observation", i.observation},
                {"context", i.context},
                {"recommended_action", i.recommended_action},
                {"type", static_cast<int>(i.type)},
                {"urgency", static_cast<int>(i.urgency)},
                {"associated_cryptofig", i.associated_cryptofig},
                {"related_capsule_ids", i.related_capsule_ids},
                {"code_file_path", i.code_file_path}
            };
        }

        static void from_json(const json& j, CerebrumLux::AIInsight& i) {
            j.at("id").get_to(i.id);
            j.at("observation").get_to(i.observation);
            j.at("context").get_to(i.context);
            j.at("recommended_action").get_to(i.recommended_action);
            i.type = static_cast<CerebrumLux::InsightType>(j.at("type").get<int>());
            i.urgency = static_cast<CerebrumLux::UrgencyLevel>(j.at("urgency").get<int>());
            j.at("associated_cryptofig").get_to(i.associated_cryptofig);
            j.at("related_capsule_ids").get_to(i.related_capsule_ids);
            j.at("code_file_path").get_to(i.code_file_path);
        }
    };
}
*/

#endif // AI_INSIGHTS_ENGINE_H