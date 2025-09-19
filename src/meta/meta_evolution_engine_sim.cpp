// File: src/meta/meta_evolution_engine_sim.cpp
#include "meta_evolution_engine.h"
#include "../core/logger.h"
#include "../data_models/dynamic_sequence.h"
#include "../data_models/sequence_manager.h"
#include "../core/utils.h" // EKLENDİ: SafeRNG için
#include <chrono>
#include <thread>
#include <sstream>

// === MetaEvolutionEngine Implementasyonları ===
// run_self_simulation metodunun implementasyonu
void MetaEvolutionEngine::run_self_simulation(int rounds, SequenceManager& seq_manager) {
    LOG_DEFAULT(LogLevel::INFO, "MetaEvolutionEngine: self-simulation başlatılıyor. rounds=" << rounds);
    
    std::uniform_real_distribution<float> u01(0.0f, 1.0f);
    std::uniform_real_distribution<float> interval_ms(20000.0f, 250000.0f); // microseconds candidate
    std::uniform_real_distribution<float> alpha_ratio(0.0f, 1.0f);

    for (int r = 0; r < rounds; ++r) {
        try {
            // 1) Create a synthetic DynamicSequence with randomized but plausible features
            DynamicSequence seq;
            seq.avg_keystroke_interval = interval_ms(SafeRNG::get_instance().get_generator()); // DEĞİŞTİRİLDİ
            seq.alphanumeric_ratio = alpha_ratio(SafeRNG::get_instance().get_generator()); // DEĞİŞTİRİLDİ
            seq.current_battery_percentage = static_cast<unsigned char>(20 + int(u01(SafeRNG::get_instance().get_generator()) * 80)); // DEĞİŞTİRİLDİ
            seq.current_battery_charging = (u01(SafeRNG::get_instance().get_generator()) > 0.7f); // DEĞİŞTİRİLDİ
            seq.statistical_features_vector = std::vector<float>(cryptofig_processor.get_autoencoder().INPUT_DIM, 0.0f);


            // Fill features with random small numbers
            for (size_t i = 0; i < seq.statistical_features_vector.size(); ++i) {
                seq.statistical_features_vector[i] = u01(SafeRNG::get_instance().get_generator()); // DEĞİŞTİRİLDİ
            }

            // 2) Process via cryptofig processor to get latent vector (if available)
            try {
                cryptofig_processor.process_sequence(seq, /*learning_rate=*/0.001f);
            } catch (const std::exception& e) {
                LOG_DEFAULT(LogLevel::WARNING, "MetaEvolutionEngine: cryptofig_processor threw during process_sequence: " << e.what());
            } catch (...) {
                LOG_DEFAULT(LogLevel::WARNING, "MetaEvolutionEngine: cryptofig_processor threw during process_sequence (unknown exception).");
            }

            // 3) Analyze intent via analyzer
            UserIntent predicted_intent = analyzer.analyze_intent(seq);
            AbstractState predicted_state = learner.infer_abstract_state(seq_manager.get_signal_buffer_copy());

            // 4) Let insights engine produce insights
            auto insights = insights_engine.generate_insights(seq);

            // 5) Evaluate / update meta-goals via goal manager
            goal_manager.evaluate_and_set_goal(seq);

            // 6) Optionally propose a minor architecture parameter suggestion (example: adjust learning rate)
            AIGoal current_goal = goal_manager.get_current_goal();
            if (seq.current_battery_percentage < 30 && current_goal == AIGoal::OptimizeProductivity) {
                LOG_DEFAULT(LogLevel::INFO, "MetaEvolutionEngine: öneri -> pil zayıf, performans-tabanlı hedefe geçici kısıtlama öneriliyor.");
            }

            // 7) Optionally run a simulated small adaptation
            if (u01(SafeRNG::get_instance().get_generator()) > 0.98f) { // DEĞİŞTİRİLDİ
                LOG_DEFAULT(LogLevel::INFO, "MetaEvolutionEngine: (sim) öneri -> küçük yapılandırma değişikliği adaylandı (insan onayı gerekir).");
            }

            // 8) Wait a short, configurable interval so long-running simulation doesn't hog CPU
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        } catch (const std::exception& e) {
            LOG_DEFAULT(LogLevel::WARNING, "MetaEvolutionEngine::run_self_simulation istisna: " << e.what());
        } catch (...) {
            LOG_DEFAULT(LogLevel::WARNING, "MetaEvolutionEngine::run_self_simulation bilinmeyen istisna.");
        }
    }
    LOG_DEFAULT(LogLevel::INFO, "MetaEvolutionEngine: self-simulation tamamlandi.");
}