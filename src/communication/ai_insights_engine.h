#ifndef CEREBRUM_LUX_AI_INSIGHTS_ENGINE_H
#define CEREBRUM_LUX_AI_INSIGHTS_ENGINE_H

#include <vector>  // For std::vector
#include <string>  // For std::wstring
#include <numeric> // For std::accumulate
#include "../core/enums.h"         // Enumlar için
#include "../core/utils.h"         // LOG_MESSAGE için
#include "../data_models/dynamic_sequence.h" // DynamicSequence için ileri bildirim
#include "../brain/intent_analyzer.h"       // IntentAnalyzer için ileri bildirim
#include "../brain/intent_learner.h"        // IntentLearner için ileri bildirim
#include "../brain/prediction_engine.h"     // PredictionEngine için ileri bildirim
#include "../brain/autoencoder.h"           // CryptofigAutoencoder için ileri bildirim
#include "../brain/cryptofig_processor.h"   // CryptofigProcessor için ileri bildirim


// İleri bildirimler
struct DynamicSequence;
class IntentAnalyzer;
class IntentLearner;
class PredictionEngine;
class CryptofigAutoencoder;
class CryptofigProcessor;

// YENİ: AIInsight struct tanımı
struct AIInsight {
    std::wstring observation;
    AIAction suggested_action = AIAction::None; // Gerekirse ilgili bir eylem önerisi
    float urgency = 0.0f; // Ne kadar acil olduğu (0.0f - 1.0f)
};

// YENİ: AIInsightsEngine sınıfı tanımı
class AIInsightsEngine {
public:
    AIInsightsEngine(IntentAnalyzer& analyzer_ref, IntentLearner& learner_ref, 
                     PredictionEngine& predictor_ref, CryptofigAutoencoder& autoencoder_ref,
                     CryptofigProcessor& cryptofig_processor_ref);

    // AI'ın mevcut iç durumunu ve performansını analiz eder ve içgörüler üretir
    std::vector<AIInsight> generate_insights(const DynamicSequence& current_sequence);

    // Bu metot public olacak (GoalManager tarafından erişim için)
    float calculate_autoencoder_reconstruction_error(const std::vector<float>& statistical_features) const;
    
    // IntentAnalyzer üyesine erişim için getter metodu (GoalManager tarafından erişim için)
    IntentAnalyzer& get_analyzer() const; 

private:
    IntentAnalyzer& analyzer;
    IntentLearner& learner;
    PredictionEngine& predictor;
    CryptofigAutoencoder& autoencoder;
    CryptofigProcessor& cryptofig_processor;

    // Yardımcı fonksiyonlar
    float calculate_average_feedback_score(UserIntent intent_id) const;
};

#endif // CEREBRUM_LUX_AI_INSIGHTS_ENGINE_H