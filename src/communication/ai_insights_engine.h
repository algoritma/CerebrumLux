#ifndef CEREBRUM_LUX_AI_INSIGHTS_ENGINE_H
#define CEREBRUM_LUX_AI_INSIGHTS_ENGINE_H

#include <vector>
#include <string>
#include <numeric>
#include <map>
#include <chrono>
#include "../core/enums.h"
#include "../core/utils.h"
#include "../data_models/dynamic_sequence.h"
#include "../brain/intent_analyzer.h"
#include "../brain/intent_learner.h"
#include "../brain/prediction_engine.h"
#include "../brain/autoencoder.h"
#include "../brain/cryptofig_processor.h"

#include "../brain/intent_learner.h"


// İleri bildirimler
struct DynamicSequence;
class IntentAnalyzer;
class IntentLearner;
class PredictionEngine;
class CryptofigAutoencoder;
class CryptofigProcessor;

// YENİ: AIInsight struct tanımı
struct AIInsight {
    std::string observation;
    AIAction suggested_action = AIAction::None;
    float urgency = 0.0f;
};

// YENİ: AIInsightsEngine sınıfı tanımı
class AIInsightsEngine {
public:
    AIInsightsEngine(IntentAnalyzer& analyzer_ref, IntentLearner& learner_ref,
                     PredictionEngine& predictor_ref, CryptofigAutoencoder& autoencoder_ref,
                     CryptofigProcessor& cryptofig_processor_ref);
    virtual std::vector<AIInsight> generate_insights(const DynamicSequence& current_sequence);

    std::string generateResponse(UserIntent intent, const std::vector<float>& latent_cryptofig_vector);

    float calculate_autoencoder_reconstruction_error(const std::vector<float>& statistical_features) const;
    
    virtual IntentAnalyzer& get_analyzer() const; 

private:
    IntentAnalyzer& analyzer;
    IntentLearner& learner;
    PredictionEngine& predictor;
    CryptofigAutoencoder& autoencoder;
    CryptofigProcessor& cryptofig_processor;

    mutable std::map<std::string, std::chrono::steady_clock::time_point> insight_cooldowns;

    // Yardımcı fonksiyonlar
    float calculate_average_feedback_score(UserIntent intent_id) const;

};

#endif // CEREBRUM_LUX_AI_INSIGHTS_ENGINE_H
