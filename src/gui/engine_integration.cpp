#include "engine_integration.h"
#include "../core/logger.h"
#include <QString>

namespace CerebrumLux {

EngineIntegration::EngineIntegration(MetaEvolutionEngine& metaEngineRef,
                                     SequenceManager& sequenceManRef,
                                     LearningModule& learningModRef,
                                     KnowledgeBase& kbRef,
                                     NaturalLanguageProcessor& nlpRef, // YENİ: NLP Referansı
                                     GoalManager& goalManRef, // YENİ: GoalManager Referansı
                                     ResponseEngine& respEngineRef, // YENİ: ResponseEngine Referansı
                                     QObject *parent)
    : QObject(parent)
    , metaEngineRef_(metaEngineRef)
    , sequenceManRef_(sequenceManRef)
    , learningModRef_(learningModRef)
    , kbRef_(kbRef)
    , nlpRef_(nlpRef) // YENİ: NLP referansı başlatıldı
    , goalManRef_(goalManRef) // YENİ: GoalManager referansı başlatıldı
    , respEngineRef_(respEngineRef) // YENİ: ResponseEngine referansı başlatıldı
{
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "EngineIntegration: Initialized.");
}

MetaEvolutionEngine& EngineIntegration::getMetaEngine() const { return metaEngineRef_; }
SequenceManager& EngineIntegration::getSequenceManager() const { return sequenceManRef_; }
LearningModule& EngineIntegration::getLearningModule() const { return learningModRef_; }
KnowledgeBase& EngineIntegration::getKnowledgeBase() const { return kbRef_; }

// YENİ: NLP, GoalManager ve ResponseEngine için getter metotları implementasyonları
NaturalLanguageProcessor& EngineIntegration::getNlpProcessor() const { return nlpRef_; }
GoalManager& EngineIntegration::getGoalManager() const { return goalManRef_; }
ResponseEngine& EngineIntegration::getResponseEngine() const { return respEngineRef_; }


void EngineIntegration::processUserCommand(const std::string& command) {
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "EngineIntegration: User command processed: " << command);
    emit simulationOutputReady(QString::fromStdString("Command received: " + command));
}

void EngineIntegration::startCoreSimulation() {
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "EngineIntegration: Core simulation started.");
    emit simulationOutputReady(QString::fromStdString("Core simulation started."));
}

void EngineIntegration::stopCoreSimulation() {
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "EngineIntegration: Core simulation stopped.");
    emit simulationOutputReady(QString::fromStdString("Core simulation stopped."));
}

} // namespace CerebrumLux