#ifndef ENGINE_INTEGRATION_H
#define ENGINE_INTEGRATION_H

#include <string>
#include <vector>
#include "../learning/KnowledgeBase.h" // KnowledgeBase için
#include "../learning/LearningModule.h" // LearningModule için
#include "../meta/meta_evolution_engine.h" // MetaEvolutionEngine için
#include "../data_models/sequence_manager.h" // SequenceManager için
#include "DataTypes.h" // SimulationData için


// İleri bildirimler
// class MetaEvolutionEngine; // Zaten dahil edildi
// class SequenceManager;   // Zaten dahil edildi
// class LearningModule;    // Zaten dahil edildi
// class KnowledgeBase;     // Zaten dahil edildi

class EngineIntegration {
public:
    // Mevcut kurucu
    EngineIntegration(MetaEvolutionEngine& meta_engine, SequenceManager& sequence_manager,
                      LearningModule& learning_module, KnowledgeBase& kb);

    // Mevcut getter'lar
    MetaEvolutionEngine& getMetaEngine() { return meta_engine; }
    SequenceManager& getSequenceManager() { return sequence_manager; }
    LearningModule& getLearningModule() { return learning_module; }
    KnowledgeBase& getKnowledgeBase() { return kb; }

    // YENİ: Simülasyonu başlatma/durdurma ve komut işleme metotları
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