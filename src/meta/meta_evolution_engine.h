#ifndef META_EVOLUTION_ENGINE_H
#define META_EVOLUTION_ENGINE_H

#include <string>
#include <vector>

// Core AI components
#include "../brain/intent_analyzer.h"
#include "../brain/intent_learner.h"
#include "../brain/prediction_engine.h"
#include "../planning_execution/goal_manager.h"
#include "../brain/cryptofig_processor.h"
#include "../communication/ai_insights_engine.h"
#include "../learning/LearningModule.h"

// YENİ EKLENDİ (run_self_simulation mantığı için gerekli)
#include "../sensors/atomic_signal.h"      // AtomicSignal tanımı için
#include "../data_models/sequence_manager.h" // SequenceManager tanımı için
#include "../brain/autoencoder.h" // CryptofigAutoencoder tanımı için

namespace CerebrumLux {

class MetaEvolutionEngine {
public:
    MetaEvolutionEngine(
        IntentAnalyzer& analyzer_ref,
        IntentLearner& learner_ref,
        PredictionEngine& predictor_ref,
        GoalManager& goal_manager_ref,
        CryptofigProcessor& cryptofig_processor_ref,
        AIInsightsEngine& insights_engine_ref,
        LearningModule& learning_module_ref
    );

    void run_meta_evolution_cycle(const DynamicSequence& current_sequence);

private:
    IntentAnalyzer& analyzer;
    IntentLearner& learner;
    PredictionEngine& predictor;
    GoalManager& goal_manager;
    CryptofigProcessor& cryptofig_processor;
    AIInsightsEngine& insights_engine;
    LearningModule& learning_module;
};

} // namespace CerebrumLux

#endif // META_EVOLUTION_ENGINE_H