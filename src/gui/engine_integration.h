#ifndef ENGINE_INTEGRATION_H
#define ENGINE_INTEGRATION_H

#include <string>
#include <vector>
#include <mutex>
#include "DataTypes.h" // eski SimulationData.h yerine tek dosya

// İleri bildirimler ve gerekli dahil etmeler
class MetaEvolutionEngine;
class SequenceManager;
class LearningModule; // YENİ: LearningModule için ileri bildirim
class KnowledgeBase;  // YENİ: KnowledgeBase için ileri bildirim

// YENİ: LearningModule'ün ve KnowledgeBase'in tam tanımı için include'lar
#include "../learning/LearningModule.h"
#include "../learning/KnowledgeBase.h"


class EngineIntegration {
public:
    // Kurucu güncellendi: LearningModule ve KnowledgeBase referansları eklendi
    EngineIntegration(MetaEvolutionEngine& meta, SequenceManager& seq, LearningModule& learner_ref, KnowledgeBase& kb_ref);

    void runSelfSimulation(int steps);
    std::string getLatestLogs();
    float getCurrentAdherenceScore();

    std::vector<SimulationData> getSimulationData();
    std::vector<LogData> getLogData();
    std::vector<GraphData> getGraphData();

    // YENİ: KnowledgeBase'e erişim için getter metodu
    KnowledgeBase& getKnowledgeBase() { return knowledgeBase; }
    const KnowledgeBase& getKnowledgeBase() const { return knowledgeBase; }


private:
    MetaEvolutionEngine& metaEngine;
    SequenceManager& sequenceManager;
    LearningModule& learningModule; // YENİ: LearningModule referansı
    KnowledgeBase& knowledgeBase;  // YENİ: KnowledgeBase referansı
    
    std::string logs;
    mutable std::mutex mtx;
};

#endif // ENGINE_INTEGRATION_H