#ifndef TUTOR_WORKER_H
#define TUTOR_WORKER_H

#include <QObject>
#include <atomic>
#include "../../learning/ai_tutor_loop.h"

// CerebrumLux::LLMEngine forward declaration
namespace CerebrumLux {
    class LLMEngine;
    class LearningModule; // Forward declaration eklendi
}

class TutorWorker : public QObject {
    Q_OBJECT

public:
    explicit TutorWorker(CerebrumLux::LLMEngine* teacher, 
                         CerebrumLux::LLMEngine* student, 
                         CerebrumLux::LearningModule* learningModule, // YENİ: LearningModule
                         const Curriculum& curriculum, 
                         QObject* parent = nullptr);

public slots:
    void run();
    void stop();

signals:
    void update(const QString& msg);
    void finished();

private:
    CerebrumLux::LLMEngine* m_teacher;
    CerebrumLux::LLMEngine* m_student;
    CerebrumLux::LearningModule* m_learningModule; // YENİ: LearningModule
    Curriculum m_curriculum;
    std::atomic<bool> m_running;
};

#endif // TUTOR_WORKER_H