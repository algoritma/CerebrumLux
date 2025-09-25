#include "meta_evolution_engine.h"
#include "../core/logger.h"
#include "../sensors/atomic_signal.h"
#include "../data_models/sequence_manager.h"
#include "../brain/autoencoder.h" // CryptofigAutoencoder tanımı için
#include "../brain/intent_analyzer.h"
#include "../brain/intent_learner.h"
#include "../communication/ai_insights_engine.h" // AIInsight tanımı için
#include "../planning_execution/goal_manager.h"
#include "../core/enums.h" // SensorType, UserIntent, AbstractState için

namespace CerebrumLux {

MetaEvolutionEngine::MetaEvolutionEngine(
    IntentAnalyzer& analyzer_ref,
    IntentLearner& learner_ref,
    PredictionEngine& predictor_ref,
    GoalManager& goal_manager_ref,
    CryptofigProcessor& cryptofig_processor_ref,
    AIInsightsEngine& insights_engine_ref,
    LearningModule& learning_module_ref
)
    : analyzer(analyzer_ref),
      learner(learner_ref),
      predictor(predictor_ref),
      goal_manager(goal_manager_ref),
      cryptofig_processor(cryptofig_processor_ref),
      insights_engine(insights_engine_ref),
      learning_module(learning_module_ref)
{
    LOG_DEFAULT(LogLevel::INFO, "MetaEvolutionEngine: Initialized.");
}

void MetaEvolutionEngine::run_meta_evolution_cycle(const DynamicSequence& current_sequence) {
    LOG_DEFAULT(LogLevel::DEBUG, "MetaEvolutionEngine: Running meta-evolution cycle...");

    int rounds = 5;
    LOG_DEFAULT(LogLevel::INFO, "MetaEvolutionEngine: Starting self-simulation for " << rounds << " rounds.");

    CerebrumLux::SequenceManager temp_sim_sequence_manager;

    for (int i = 0; i < rounds; ++i) {
        LOG_DEFAULT(LogLevel::DEBUG, "MetaEvolutionEngine: Self-simulation round " << (i + 1));

        CerebrumLux::AtomicSignal simulated_signal;
        simulated_signal.type = CerebrumLux::SensorType::Keyboard;
        simulated_signal.value = "sim_input_" + std::to_string(i);
        temp_sim_sequence_manager.add_signal(simulated_signal, this->cryptofig_processor); // Eksik argüman eklendi

        CerebrumLux::DynamicSequence simulated_sequence;
        simulated_sequence.id = "sim_seq_" + std::to_string(i);
        simulated_sequence.timestamp_utc = std::chrono::system_clock::now();
        simulated_sequence.event_count = i + 1;

        simulated_sequence.statistical_features_vector.assign(CerebrumLux::CryptofigAutoencoder::INPUT_DIM, 0.0f);
        for (size_t j = 0; j < simulated_sequence.statistical_features_vector.size(); ++j) {
            simulated_sequence.statistical_features_vector[j] = static_cast<float>(rand()) / RAND_MAX;
        }

        this->cryptofig_processor.process_sequence(simulated_sequence, 0.01f);

        CerebrumLux::UserIntent predicted_intent = this->analyzer.analyze_intent(simulated_sequence);
        CerebrumLux::AbstractState inferred_state = this->learner.infer_abstract_state(temp_sim_sequence_manager.get_signal_buffer_copy());

        std::vector<CerebrumLux::AIInsight> insights = this->insights_engine.generate_insights(simulated_sequence);
        if (!insights.empty()) {
            this->learning_module.process_ai_insights(insights);
        }

        this->goal_manager.evaluate_and_set_goal(simulated_sequence);

        LOG_DEFAULT(LogLevel::DEBUG, "MetaEvolutionEngine: Simulated intent: " << static_cast<int>(predicted_intent)
                                      << ", Inferred state: " << static_cast<int>(inferred_state));
    }
    LOG_DEFAULT(LogLevel::INFO, "MetaEvolutionEngine: Self-simulation finished.");

    LOG_DEFAULT(LogLevel::DEBUG, "MetaEvolutionEngine: Meta-evolution cycle completed.");
}

} // namespace CerebrumLux