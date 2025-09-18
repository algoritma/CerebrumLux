#include "../learning/LearningModule.h"
#include "../learning/KnowledgeBase.h"
#include "../communication/ai_insights_engine.h"  // YENİ: Doğru yol ile AIInsightsEngine dahil edildi

class EngineIntegration {
public:
    EngineIntegration();
    
    void updateAIInsights();  // Döngü içinde çağrılacak

private:
    KnowledgeBase knowledgeBase;
    LearningModule learningModule;
    AIInsightsEngine insightsEngine;  // İçgörüler için
};