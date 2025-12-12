#ifndef TRAINING_HUB_PANEL_H
#define TRAINING_HUB_PANEL_H

#include <QWidget>

namespace CerebrumLux {
    class LLMEngine;
    class LearningModule; // Forward declaration eklendi
}

class QThread; // Forward declaration
class CurriculumPanel;
class TutorPanel;
class TutorWorker; // Forward declaration

class TrainingHubPanel : public QWidget {
    Q_OBJECT

public:
    explicit TrainingHubPanel(CerebrumLux::LLMEngine* teacherModel, 
                              CerebrumLux::LLMEngine* studentModel, 
                              CerebrumLux::LearningModule* learningModule, // YENİ
                              QWidget *parent = nullptr);
    ~TrainingHubPanel();

private:
    void setupUi();

    CurriculumPanel* m_curriculumPanel;
    TutorPanel* m_tutorPanel;

    CerebrumLux::LLMEngine* m_teacherModel;
    CerebrumLux::LLMEngine* m_studentModel;
    CerebrumLux::LearningModule* m_learningModule; // YENİ

    // Thread ve Worker yönetimi için üye değişkenler
    TutorWorker* m_worker = nullptr;
    QThread* m_workerThread = nullptr;
    
};

#endif // TRAINING_HUB_PANEL_H
