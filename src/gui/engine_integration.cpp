#include "engine_integration.h"
#include "../core/logger.h"

namespace CerebrumLux {

EngineIntegration::EngineIntegration(MetaEvolutionEngine& metaEngineRef,
                                     SequenceManager& sequenceManRef,
                                     LearningModule& learningModRef,
                                     KnowledgeBase& kbRef,
                                     QObject *parent)
    : QObject(parent),
      metaEngineRef(metaEngineRef),
      sequenceManRef(sequenceManRef),
      learningModRef(learningModRef),
      kbRef(kbRef)
{
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "EngineIntegration: Initialized.");
}

void EngineIntegration::processUserCommand(const std::string& command) {
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "EngineIntegration: User command processed: " << command);
    emit simulationOutputReady(QString::fromStdString("Command received: " + command));
}

void EngineIntegration::startCoreSimulation() {
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "EngineIntegration: Core simulation started.");
    emit simulationOutputReady("Core simulation started.");
}

void EngineIntegration::stopCoreSimulation() {
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "EngineIntegration: Core simulation stopped.");
    emit simulationOutputReady("Core simulation stopped.");
}

} // namespace CerebrumLux