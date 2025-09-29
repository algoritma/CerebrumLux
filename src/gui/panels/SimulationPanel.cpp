#include "SimulationPanel.h"
#include "../../core/logger.h" // LOG_DEFAULT makrosu için
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView> // QTableView başlıkları için
#include <QMessageBox> // Hata mesajları için
#include <QDateTime> // QDateTime için
#include <QScrollBar> // QTextEdit'in kaydırma çubuğu için
#include <QLabel> // YENİ: QLabel'in tam tanımı için eklendi

namespace CerebrumLux {

// Kurucu
CerebrumLux::SimulationPanel::SimulationPanel(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Komut giriş alanı
    QHBoxLayout *commandLayout = new QHBoxLayout();
    this->commandLineEdit = new QLineEdit(this);
    this->commandLineEdit->setPlaceholderText("Simülasyon komutunu girin...");
    commandLayout->addWidget(this->commandLineEdit);

    this->sendCommandButton = new QPushButton("Komut Gönder", this);
    commandLayout->addWidget(this->sendCommandButton);
    mainLayout->addLayout(commandLayout);

    connect(this->commandLineEdit, &QLineEdit::returnPressed, this, &CerebrumLux::SimulationPanel::onCommandLineEditReturnPressed);
    connect(this->sendCommandButton, &QPushButton::clicked, this, &CerebrumLux::SimulationPanel::onCommandLineEditReturnPressed);

    // Kontrol düğmeleri
    QHBoxLayout *controlLayout = new QHBoxLayout();
    this->startSimulationButton = new QPushButton("Simülasyonu Başlat", this);
    controlLayout->addWidget(this->startSimulationButton);
    this->stopSimulationButton = new QPushButton("Simülasyonu Durdur", this);
    controlLayout->addWidget(this->stopSimulationButton);
    mainLayout->addLayout(controlLayout);

    connect(this->startSimulationButton, &QPushButton::clicked, this, &CerebrumLux::SimulationPanel::onStartSimulationClicked);
    connect(this->stopSimulationButton, &QPushButton::clicked, this, &CerebrumLux::SimulationPanel::onStopSimulationClicked);

    // Simülasyon tarihçesi gösterimi
    this->simulationHistoryTable = new QTableView(this);
    this->simulationHistoryModel = new QStandardItemModel(0, 3, this);
    this->simulationHistoryModel->setHeaderData(0, Qt::Horizontal, "ID");
    this->simulationHistoryModel->setHeaderData(1, Qt::Horizontal, "Değer");
    this->simulationHistoryModel->setHeaderData(2, Qt::Horizontal, "Zaman Damgası");
    this->simulationHistoryTable->setModel(this->simulationHistoryModel);
    this->simulationHistoryTable->horizontalHeader()->setStretchLastSection(true);
    mainLayout->addWidget(this->simulationHistoryTable);

    // --- YENİ: Chat Arayüzü ---
    mainLayout->addSpacing(10); // Biraz boşluk bırak
    mainLayout->addWidget(new QLabel("Chat:", this));

    chatHistoryDisplay = new QTextEdit(this);
    chatHistoryDisplay->setReadOnly(true);
    chatHistoryDisplay->setMinimumHeight(150);
    mainLayout->addWidget(chatHistoryDisplay);

    QHBoxLayout *chatInputLayout = new QHBoxLayout();
    chatMessageLineEdit = new QLineEdit(this);
    chatMessageLineEdit->setPlaceholderText("Buraya mesajınızı yazın...");
    chatInputLayout->addWidget(chatMessageLineEdit);

    sendChatMessageButton = new QPushButton("Gönder", this);
    chatInputLayout->addWidget(sendChatMessageButton);

    mainLayout->addLayout(chatInputLayout);

    connect(chatMessageLineEdit, &QLineEdit::returnPressed, this, &CerebrumLux::SimulationPanel::onChatMessageLineEditReturnPressed);
    connect(sendChatMessageButton, &QPushButton::clicked, this, &CerebrumLux::SimulationPanel::onChatMessageLineEditReturnPressed);

    // Chat geçmişinin otomatik aşağı kayması için
    connect(chatHistoryDisplay->verticalScrollBar(), &QScrollBar::rangeChanged,
            [this](int min, int max){ Q_UNUSED(min); chatHistoryDisplay->verticalScrollBar()->setValue(max); });


    setLayout(mainLayout);

    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "SimulationPanel: Initialized with input and control buttons.");
}

// updateSimulationHistory metodu
void CerebrumLux::SimulationPanel::updateSimulationHistory(const QVector<CerebrumLux::SimulationData>& data) {
    this->simulationHistoryModel->setRowCount(0);
    for (const auto& item : data) {
        QList<QStandardItem*> rowItems;
        rowItems.append(new QStandardItem(item.id));
        rowItems.append(new QStandardItem(QString::number(item.value, 'f', 2)));
        rowItems.append(new QStandardItem(QDateTime::fromSecsSinceEpoch(item.timestamp).toString("hh:mm:ss")));
        this->simulationHistoryModel->appendRow(rowItems);
    }
    this->simulationHistoryTable->scrollToBottom();
}

// YENİ: Chat mesajı ekleme slotu
void CerebrumLux::SimulationPanel::appendChatMessage(const QString& sender, const CerebrumLux::ChatResponse& chatResponse) {
    // Chat geçmişini HTML olarak oluşturacağız
    QString messageHtml = chatHistoryDisplay->toHtml();
    
    // Zaman damgasını al
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");

    // Mesajı sender'a göre farklı renklendir ve HTML formatında ekle
    if (sender == "CerebrumLux") {
        messageHtml += QString("<p style=\"margin-bottom:0;\"><b><font color=\"#007bff\">%1 %2:</font></b> %3</p>")
                       .arg(timestamp).arg(sender).arg(QString::fromStdString(chatResponse.text));
        
        // Gerekçeyi daha küçük ve soluk renkte ekle (eğer varsa)
        if (!chatResponse.reasoning.empty()) {
            messageHtml += QString("<p style=\"margin-left:20px; font-size:0.8em; color:#6c757d; margin-top:0;\"><i>Gerekçe: %1</i></p>")
                           .arg(QString::fromStdString(chatResponse.reasoning));
        }
        // Açıklama gerekliyse bir uyarı ekle
        if (chatResponse.needs_clarification) {
            messageHtml += QString("<p style=\"margin-left:20px; font-size:0.8em; color:#ffc107; margin-top:0;\"><i>(Bu yanıt belirsiz olabilir, ek bilgi sağlamak ister misiniz?)</i></p>");
        }
    } else { // Kullanıcı mesajı
        messageHtml += QString("<p><b><font color=\"#28a745\">%1 %2:</font></b> %3</p>")
                       .arg(timestamp).arg(sender).arg(QString::fromStdString(chatResponse.text));
    }

    chatHistoryDisplay->setHtml(messageHtml);

    // Otomatik olarak en alta kaydır
    QScrollBar *sb = chatHistoryDisplay->verticalScrollBar();
    if (sb) {
        sb->setValue(sb->maximum());
    }
}


// onCommandLineEditReturnPressed slotu
void CerebrumLux::SimulationPanel::onCommandLineEditReturnPressed() {
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "SimulationPanel: onCommandLineEditReturnPressed slot triggered.");
    QString command = this->commandLineEdit->text();
    if (!command.isEmpty()) {
        LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "SimulationPanel: Command entered: " << command.toStdString());
        emit commandEntered(command);
        this->commandLineEdit->clear();
    } else {
        LOG_DEFAULT(CerebrumLux::LogLevel::WARNING, "SimulationPanel: Boş komut girildi.");
    }
}

// onStartSimulationClicked slotu
void CerebrumLux::SimulationPanel::onStartSimulationClicked() {
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "SimulationPanel: Start Simulation button clicked.");
    emit startSimulationTriggered();
}

// onStopSimulationClicked slotu
void CerebrumLux::SimulationPanel::onStopSimulationClicked() {
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "SimulationPanel: Stop Simulation button clicked.");
    emit stopSimulationTriggered();
}

// YENİ: Chat mesajı girişi için slot
void CerebrumLux::SimulationPanel::onChatMessageLineEditReturnPressed() {
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "SimulationPanel: onChatMessageLineEditReturnPressed slot triggered.");
    QString message = chatMessageLineEdit->text();
    if (!message.isEmpty()) {
        // Kullanıcı mesajı için bir ChatResponse objesi oluştur
        CerebrumLux::ChatResponse userChatResponse;
        userChatResponse.text = message.toStdString();
        appendChatMessage("User", userChatResponse); // Yeni imzaya uygun çağrı
        emit chatMessageEntered(message);
        chatMessageLineEdit->clear();
    } else {
        LOG_DEFAULT(CerebrumLux::LogLevel::WARNING, "SimulationPanel: Boş chat mesajı girildi.");
    }
}

} // namespace CerebrumLux