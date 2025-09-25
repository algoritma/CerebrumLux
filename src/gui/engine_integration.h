#ifndef ENGINE_INTEGRATION_H
#define ENGINE_INTEGRATION_H

#include <QObject>
#include <QString>
#include <QVariant>

// CerebrumLux core başlıkları
#include "../meta/meta_evolution_engine.h"
#include "../data_models/sequence_manager.h"
#include "../learning/LearningModule.h"
#include "../learning/KnowledgeBase.h"

namespace CerebrumLux {

class EngineIntegration : public QObject {
    Q_OBJECT
public:
    explicit EngineIntegration(MetaEvolutionEngine& metaEngineRef,
                               SequenceManager& sequenceManRef,
                               LearningModule& learningModRef,
                               KnowledgeBase& kbRef,
                               QObject *parent = nullptr);

    virtual ~EngineIntegration() = default;

    MetaEvolutionEngine& getMetaEvolutionEngine() const { return metaEngineRef; }
    SequenceManager& getSequenceManager() const { return sequenceManRef; }
    LearningModule& getLearningModule() const { return learningModRef; }
    KnowledgeBase& getKnowledgeBase() const { return kbRef; }

    void processUserCommand(const std::string& command);
    void startCoreSimulation();
    void stopCoreSimulation();

signals:
    void simulationOutputReady(const QString& output);

private:
    MetaEvolutionEngine& metaEngineRef;
    SequenceManager& sequenceManRef;
    LearningModule& learningModRef;
    KnowledgeBase& kbRef;
};

} // namespace CerebrumLux

#endif // ENGINE_INTEGRATION_H