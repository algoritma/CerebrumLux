#ifndef ENGINE_INTEGRATION_H
#define ENGINE_INTEGRATION_H

#include <QObject>
#include <QString>
#include <string>

#include "../meta/meta_evolution_engine.h"
#include "../data_models/sequence_manager.h"
#include "../learning/LearningModule.h"
#include "../learning/KnowledgeBase.h"
#include "../communication/natural_language_processor.h" // YENİ: NLP için
#include "../planning_execution/goal_manager.h" // YENİ: GoalManager için
#include "../communication/response_engine.h" // YENİ: ResponseEngine için


namespace CerebrumLux {

class EngineIntegration : public QObject {
Q_OBJECT
public:
    explicit EngineIntegration(MetaEvolutionEngine& metaEngineRef,
                               SequenceManager& sequenceManRef,
                               LearningModule& learningModRef,
                               KnowledgeBase& kbRef,
                               NaturalLanguageProcessor& nlpRef, // YENİ: NLP Referansı
                               GoalManager& goalManRef, // YENİ: GoalManager Referansı
                               ResponseEngine& respEngineRef, // YENİ: ResponseEngine Referansı
                               QObject *parent = nullptr);

    virtual ~EngineIntegration() = default;

    MetaEvolutionEngine& getMetaEngine() const;
    SequenceManager& getSequenceManager() const;
    LearningModule& getLearningModule() const;
    KnowledgeBase& getKnowledgeBase() const;
    
    // YENİ: NLP, GoalManager ve ResponseEngine için getter metotları
    NaturalLanguageProcessor& getNlpProcessor() const;
    GoalManager& getGoalManager() const;
    ResponseEngine& getResponseEngine() const;

public slots:
    void processUserCommand(const std::string& command);
    void startCoreSimulation();
    void stopCoreSimulation();

signals:
    void simulationOutputReady(const QString& output);

private:
    MetaEvolutionEngine& metaEngineRef_;
    SequenceManager& sequenceManRef_;
    LearningModule& learningModRef_;
    KnowledgeBase& kbRef_;
    NaturalLanguageProcessor& nlpRef_; // YENİ: NLP Referansı üyesi
    GoalManager& goalManRef_; // YENİ: GoalManager Referansı üyesi
    ResponseEngine& respEngineRef_; // YENİ: ResponseEngine Referansı üyesi
};

} // namespace CerebrumLux

#endif // ENGINE_INTEGRATION_H