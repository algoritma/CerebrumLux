#ifndef LEARNINGMODULE_H
#define LEARNINGMODULE_H

#include <string>
#include <vector> // std::vector için
#include "KnowledgeBase.h"
#include "../communication/ai_insights_engine.h" // YENİ: AIInsight için dahil edildi

class LearningModule {
public:
    LearningModule(KnowledgeBase& kb) : knowledgeBase(kb) {}

    void learnFromText(const std::string& text,
                       const std::string& source,
                       const std::string& topic,
                       float confidence = 1.0f);

    void learnFromWeb(const std::string& query); // placeholder

    std::vector<Capsule> getCapsulesByTopic(const std::string& topic) {
        return knowledgeBase.getCapsulesByTopic(topic);
    }

    // YENİ: AI Insights verisini işleme metodu
    void process_ai_insights(const std::vector<AIInsight>& insights);

    // YENİ: KnowledgeBase'e erişim için getter metodu
    KnowledgeBase& getKnowledgeBase() { return knowledgeBase; }
    const KnowledgeBase& getKnowledgeBase() const { return knowledgeBase; }


private:
    KnowledgeBase& knowledgeBase;
};

#endif // LEARNINGMODULE_H