#ifndef ENGINE_INTEGRATION_H
#define ENGINE_INTEGRATION_H

#include <string>
#include <vector>
#include "../learning/KnowledgeBase.h" 
#include "../learning/LearningModule.h" 
#include "../meta/meta_evolution_engine.h" 
#include "../data_models/sequence_manager.h" 
#include "DataTypes.h" 

class EngineIntegration {
public:
    EngineIntegration(MetaEvolutionEngine& meta_engine, SequenceManager& sequence_manager,
                      LearningModule& learning_module, KnowledgeBase& kb);

    MetaEvolutionEngine& getMetaEngine() { return meta_engine; }
    SequenceManager& getSequenceManager() { return sequence_manager; }
    LearningModule& getLearningModule() { return learning_module; }
    KnowledgeBase& getKnowledgeBase() { return kb; }

    void startCoreSimulation();
    void stopCoreSimulation();
    void processUserCommand(const std::string& command);

private:
    MetaEvolutionEngine& meta_engine;
    SequenceManager& sequence_manager;
    LearningModule& learning_module;
    KnowledgeBase& kb;
};

#endif // ENGINE_INTEGRATION_H