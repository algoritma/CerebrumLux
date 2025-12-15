#include "TutorPanel.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QPlainTextEdit>
#include <QScrollBar>
#include "core/logger.h"

TutorPanel::TutorPanel(QWidget *parent) : QWidget(parent) {
    auto layout = new QVBoxLayout(this);

    m_statusLabel = new QLabel("Durum: Hazır", this);
    m_statusLabel->setStyleSheet("font-weight: bold;");
    layout->addWidget(m_statusLabel);

    auto btnLayout = new QHBoxLayout();
    m_startButton = new QPushButton("Eğitimi Başlat", this);
    m_startButton->setStyleSheet("background-color: #4CAF50; color: white;");
    connect(m_startButton, &QPushButton::clicked, [this](){ 
        LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "TutorPanel: Start button clicked.");
        m_trainingLog->clear(); 
        emit startTrainingClicked(); 
        m_startButton->setEnabled(false);
        m_stopButton->setEnabled(true);
        m_statusLabel->setText("Durum: Çalışıyor...");
    });
    
    m_stopButton = new QPushButton("Durdur", this);
    m_stopButton->setStyleSheet("background-color: #f44336; color: white;");
    m_stopButton->setEnabled(false);
    connect(m_stopButton, &QPushButton::clicked, [this](){
        emit stopTrainingClicked();
        m_stopButton->setEnabled(false); // Tekrar basılamasın
        m_statusLabel->setText("Durum: Durduruluyor...");
    });
    
    btnLayout->addWidget(m_startButton);
    btnLayout->addWidget(m_stopButton);
    layout->addLayout(btnLayout);

    m_trainingLog = new QPlainTextEdit(this);
    m_trainingLog->setReadOnly(true);
    QFont f("Courier"); f.setStyleHint(QFont::Monospace);
    m_trainingLog->setFont(f);
    layout->addWidget(m_trainingLog);
}

TutorPanel::~TutorPanel() {}

void TutorPanel::handleTrainingUpdate(const QString& update) {
    m_trainingLog->appendHtml(update);
    m_trainingLog->verticalScrollBar()->setValue(m_trainingLog->verticalScrollBar()->maximum());
}

void TutorPanel::handleTrainingFinished() {
    m_statusLabel->setText("Durum: Bitti / Durduruldu");
    m_startButton->setEnabled(true);
    m_stopButton->setEnabled(false);
}