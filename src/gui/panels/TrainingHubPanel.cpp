#include "TrainingHubPanel.h"
#include "CurriculumPanel.h"
#include "TutorPanel.h"
#include "TutorWorker.h"
#include <QVBoxLayout>
#include <QSplitter>
#include <QThread>

TrainingHubPanel::TrainingHubPanel(CerebrumLux::LLMEngine* teacherModel, 
                                   CerebrumLux::LLMEngine* studentModel,
                                   CerebrumLux::LearningModule* learningModule,
                                   QWidget *parent)
    : QWidget(parent), m_teacherModel(teacherModel), m_studentModel(studentModel), 
      m_learningModule(learningModule),
      m_worker(nullptr), m_workerThread(nullptr)
{
    setupUi();
}

TrainingHubPanel::~TrainingHubPanel()
{
}

void TrainingHubPanel::setupUi() {
    auto mainLayout = new QHBoxLayout(this);
    auto splitter = new QSplitter(Qt::Horizontal, this);

    // Left side: Curriculum management
    m_curriculumPanel = new CurriculumPanel(this);

    // Right side: Training execution and log
    m_tutorPanel = new TutorPanel(this);

    splitter->addWidget(m_curriculumPanel);
    splitter->addWidget(m_tutorPanel);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);

    mainLayout->addWidget(splitter);

    // --- Worker and Thread Management ---
    connect(m_tutorPanel, &TutorPanel::startTrainingClicked, this, [this]() {
        // Eğer zaten çalışan bir thread varsa, yenisini başlatma
        if (m_workerThread && m_workerThread->isRunning()) {
            return;
        }
        Curriculum curriculum = m_curriculumPanel->getCurriculum();
        
        // Yeni Thread ve Worker oluştur
        m_workerThread = new QThread(this);
        // LearningModule'ü de geçiriyoruz
        m_worker = new TutorWorker(m_teacherModel, m_studentModel, m_learningModule, curriculum);
        m_worker->moveToThread(m_workerThread);

        // Connect worker signals to TutorPanel slots
        connect(m_workerThread, &QThread::started, m_worker, &TutorWorker::run);
        connect(m_worker, &TutorWorker::update, m_tutorPanel, &TutorPanel::handleTrainingUpdate);
        connect(m_worker, &TutorWorker::finished, m_tutorPanel, &TutorPanel::handleTrainingFinished);
        
        // İşlem bittiğinde thread'i durdur
        connect(m_worker, &TutorWorker::finished, m_workerThread, &QThread::quit);

        // Temizlik: Worker ve Thread işleri bitince silinsin
        connect(m_worker, &TutorWorker::finished, m_worker, &TutorWorker::deleteLater);
        connect(m_workerThread, &QThread::finished, m_workerThread, &QThread::deleteLater);
        
        // Thread tamamen bittiğinde pointerları sıfırla
        connect(m_workerThread, &QThread::finished, this, [this]() {
            m_workerThread = nullptr;
            m_worker = nullptr;
        });
        
        m_workerThread->start();
   });

    connect(m_tutorPanel, &TutorPanel::stopTrainingClicked, this, [this]() {
        if(m_worker) {
            m_worker->stop(); // Tell the worker to stop its loop
        }
    });

    // When the hub is destroyed, make sure to quit the thread if it's running
    connect(this, &QObject::destroyed, this, [=](){
        if(m_workerThread && m_workerThread->isRunning()){
            m_worker->stop(); // Önce döngüyü kır
            m_workerThread->quit();
            m_workerThread->wait();
        }
    });
}
