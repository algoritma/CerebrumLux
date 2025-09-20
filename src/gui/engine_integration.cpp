#include "engine_integration.h"
#include "../core/logger.h" // Loglama için
// AI Core bağımlılıkları (gerekirse buraya dahil edilebilir, şimdilik sadece Logger ile logluyoruz)
// Örneğin, eğer startCoreSimulation, SequenceManager'dan bir metot çağıracaksa
// #include "../data_models/sequence_manager.h"

// Kurucu
EngineIntegration::EngineIntegration(MetaEvolutionEngine& meta_engine_ref, SequenceManager& sequence_manager_ref,
                                     LearningModule& learning_module_ref, KnowledgeBase& kb_ref)
    : meta_engine(meta_engine_ref), sequence_manager(sequence_manager_ref),
      learning_module(learning_module_ref), kb(kb_ref)
{
    LOG(LogLevel::INFO, "EngineIntegration: Initialized.");
}

// YENİ: Simülasyonu başlatma metodu
void EngineIntegration::startCoreSimulation() {
    LOG(LogLevel::INFO, "EngineIntegration: Core simulation started.");
    // TODO: Burada Cerebrum Lux'ın ana simülasyon döngüsünü başlatan gerçek mantık gelecek.
    // Örneğin: meta_engine.run_meta_evolution_cycle(); (zaten QTimer ile çağrılıyor, belki farklı bir başlatma mekanizması)
    // veya sequence_manager.startCollectingData(); gibi.
}

// YENİ: Simülasyonu durdurma metodu
void EngineIntegration::stopCoreSimulation() {
    LOG(LogLevel::INFO, "EngineIntegration: Core simulation stopped.");
    // TODO: Burada Cerebrum Lux'ın ana simülasyon döngüsünü durduran gerçek mantık gelecek.
    // Örneğin: meta_engine.stopMetaEvolution(); veya sequence_manager.stopCollectingData(); gibi.
}

// YENİ: Kullanıcı komutunu işleme metodu
void EngineIntegration::processUserCommand(const std::string& command) {
    LOG(LogLevel::INFO, "EngineIntegration: Processing user command: '" << command << "'");
    // TODO: Burada gelen komutu analiz edip Cerebrum Lux'ın ilgili modüllerine yönlendirme mantığı gelecek.
    // Örneğin:
    // if (command == "optimize performance") {
    //     meta_engine.set_meta_goal(AIGoal::OptimizePerformance);
    // } else if (command == "learn faster") {
    //     learning_module.adjustLearningRate(0.1f);
    // }
    // NlpProcessor veya IntentAnalyzer gibi modüller de burada kullanılabilir.
}