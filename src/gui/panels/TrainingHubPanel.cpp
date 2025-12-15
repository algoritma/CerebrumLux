#include "TrainingHubPanel.h"
#include "CurriculumPanel.h"
#include "TutorPanel.h"
#include "ai_tutor/tutor_broker_router.h" // YENİ
#include "brain/llm_engine.h" // LLMEngine için
#include <QVBoxLayout>
#include <QSplitter>
#include <QDebug>

TrainingHubPanel::TrainingHubPanel(CerebrumLux::LLMEngine* teacherModel,
                                   CerebrumLux::LLMEngine* studentModel,
                                   CerebrumLux::LearningModule* learningModule,
                                   QWidget *parent)
    : QWidget(parent), m_teacherModel(teacherModel), m_studentModel(studentModel),
      m_learningModule(learningModule), m_tutorBroker(nullptr)
{
    setupUi();
}

TrainingHubPanel::~TrainingHubPanel()
{
    // Broker'ı durdur ve temizle
    if (m_tutorBroker) {
        m_tutorBroker->stop();
        delete m_tutorBroker;
    }
}

void TrainingHubPanel::setupUi() {
    auto mainLayout = new QHBoxLayout(this);
    auto splitter = new QSplitter(Qt::Horizontal, this);

    m_curriculumPanel = new CurriculumPanel(this);
    m_tutorPanel = new TutorPanel(this);

    splitter->addWidget(m_curriculumPanel);
    splitter->addWidget(m_tutorPanel);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);

    mainLayout->addWidget(splitter);

    // --- Broker Yönetimi ---
    m_tutorBroker = new TutorBroker();

    // LLM'leri ve diğer servisleri Broker'a bağla
    if(m_teacherModel) {
        // TeacherAI ve StudentAI'nin Llama'yı kullanmasını sağlayacak adaptörler
        // Şimdilik basit bir sohbet fonksiyonu bağlıyoruz.
        // Gerçek implementasyonda prompt'u daha detaylı işlemek gerekebilir.
        auto llm_callback = [this](const std::string& prompt) -> std::string {
            CerebrumLux::LLMGenerationConfig config;
            // config ayarları...
            return m_teacherModel->generate(prompt, config, nullptr);
        };
        m_tutorBroker->set_external_llm(llm_callback);
    }
    // NOT: FastText veya diğer intent classifier'lar da burada set edilebilir.
    // m_tutorBroker->set_fasttext_classifier(...)

    // GUI sinyallerini Broker'a bağla
    connect(m_tutorPanel, &TutorPanel::startTrainingClicked, this, [this]() {
        // Müfredatı panelden alıp globale yükle
        const CerebrumLux::Curriculum& curriculum = m_curriculumPanel->getCurriculum();
        for(const auto& pair : curriculum.sections) {
            CerebrumLux::GLOBAL_CURRICULUM.addSection(pair.first, pair.second);
        }
        
        m_tutorPanel->handleTrainingUpdate("Tutor broker başlatılıyor...");
        m_tutorBroker->start();
    });

    connect(m_tutorPanel, &TutorPanel::stopTrainingClicked, this, [this]() {
        if(m_tutorBroker) {
            m_tutorBroker->stop();
            m_tutorPanel->handleTrainingUpdate("Tutor broker durduruluyor...");
            m_tutorPanel->handleTrainingFinished(); // Butonları resetle
        }
    });

    // Broker'dan gelen mesajları GUI'de göster
    connect(m_tutorBroker, &TutorBroker::new_message_for_gui, this, [this](const Msg& msg){
        QString displayText = QString("<b>[%1]</b>: %2")
            .arg(QString::fromStdString(msg.from))
            .arg(QString::fromStdString(msg.payload.value("text", "")));
        
        m_tutorPanel->handleTrainingUpdate(displayText);

        // Hata mesajı varsa durumu güncelle
        if (msg.type == "error") {
            m_tutorPanel->handleTrainingFinished(); // Hata durumunda da durmuş gibi göster
        }
    });
    
    // Pencere kapandığında broker'ı temizle
    connect(this, &QObject::destroyed, this, [=](){
        if(m_tutorBroker){
            m_tutorBroker->stop();
        }
    });
}
