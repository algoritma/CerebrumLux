#include "SimulationPanel.h"
#include "../../core/logger.h" // LOG_DEFAULT makrosu için
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView> // QTableView başlıkları için
#include <QMessageBox> // Hata mesajları için
#include <QDateTime> // QDateTime için

namespace CerebrumLux {

// Kurucu
CerebrumLux::SimulationPanel::SimulationPanel(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Komut giriş alanı
    QHBoxLayout *commandLayout = new QHBoxLayout();
    this->commandLineEdit = new QLineEdit(this); // 'this->' eklendi
    this->commandLineEdit->setPlaceholderText("Simülasyon komutunu girin..."); // 'this->' eklendi
    commandLayout->addWidget(this->commandLineEdit); // 'this->' eklendi

    this->sendCommandButton = new QPushButton("Komut Gönder", this); // 'this->' eklendi
    commandLayout->addWidget(this->sendCommandButton); // 'this->' eklendi
    mainLayout->addLayout(commandLayout);

    connect(this->commandLineEdit, &QLineEdit::returnPressed, this, &CerebrumLux::SimulationPanel::onCommandLineEditReturnPressed); // 'this->' ve namespace eklendi
    connect(this->sendCommandButton, &QPushButton::clicked, this, &CerebrumLux::SimulationPanel::onCommandLineEditReturnPressed); // 'this->' ve namespace eklendi

    // Kontrol düğmeleri
    QHBoxLayout *controlLayout = new QHBoxLayout();
    this->startSimulationButton = new QPushButton("Simülasyonu Başlat", this); // 'this->' eklendi
    controlLayout->addWidget(this->startSimulationButton); // 'this->' eklendi
    this->stopSimulationButton = new QPushButton("Simülasyonu Durdur", this); // 'this->' eklendi
    controlLayout->addWidget(this->stopSimulationButton); // 'this->' eklendi
    mainLayout->addLayout(controlLayout);

    connect(this->startSimulationButton, &QPushButton::clicked, this, &CerebrumLux::SimulationPanel::onStartSimulationClicked); // 'this->' ve namespace eklendi
    connect(this->stopSimulationButton, &QPushButton::clicked, this, &CerebrumLux::SimulationPanel::onStopSimulationClicked); // 'this->' ve namespace eklendi

    // Simülasyon tarihçesi gösterimi
    this->simulationHistoryTable = new QTableView(this); // 'this->' eklendi
    this->simulationHistoryModel = new QStandardItemModel(0, 3, this); // 'this->' eklendi
    this->simulationHistoryModel->setHeaderData(0, Qt::Horizontal, "ID");
    this->simulationHistoryModel->setHeaderData(1, Qt::Horizontal, "Değer");
    this->simulationHistoryModel->setHeaderData(2, Qt::Horizontal, "Zaman Damgası");
    this->simulationHistoryTable->setModel(this->simulationHistoryModel); // 'this->' eklendi
    this->simulationHistoryTable->horizontalHeader()->setStretchLastSection(true); // 'this->' eklendi
    mainLayout->addWidget(this->simulationHistoryTable); // 'this->' eklendi

    setLayout(mainLayout);

    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "SimulationPanel: Initialized with input and control buttons.");
}

// updateSimulationHistory metodu
void CerebrumLux::SimulationPanel::updateSimulationHistory(const QVector<CerebrumLux::SimulationData>& data) {
    this->simulationHistoryModel->setRowCount(0); // Eski verileri temizle // 'this->' eklendi
    for (const auto& item : data) {
        QList<QStandardItem*> rowItems;
        rowItems.append(new QStandardItem(item.id));
        rowItems.append(new QStandardItem(QString::number(item.value, 'f', 2)));
        rowItems.append(new QStandardItem(QDateTime::fromSecsSinceEpoch(item.timestamp).toString("hh:mm:ss")));
        this->simulationHistoryModel->appendRow(rowItems); // 'this->' eklendi
    }
    this->simulationHistoryTable->scrollToBottom(); // Tabloyu en alta kaydır // 'this->' eklendi
}

// onCommandLineEditReturnPressed slotu
void CerebrumLux::SimulationPanel::onCommandLineEditReturnPressed() {
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "SimulationPanel: onCommandLineEditReturnPressed slot triggered.");
    QString command = this->commandLineEdit->text(); // 'this->' eklendi
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "SimulationPanel: CommandLineEdit text: '" << command.toStdString() << "' (isEmpty: " << command.isEmpty() << ")");

    if (!command.isEmpty()) {
        LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "SimulationPanel: Command entered: " << command.toStdString());
        emit commandEntered(command);
        this->commandLineEdit->clear(); // Komut gönderildikten sonra temizle // 'this->' eklendi
    } else {
        LOG_DEFAULT(CerebrumLux::LogLevel::WARNING, "SimulationPanel: Command entered was empty.");
    }
}

// onStartSimulationClicked slotu
void CerebrumLux::SimulationPanel::onStartSimulationClicked() {
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "SimulationPanel: Start Simulation button clicked.");
    emit startSimulationTriggered(); // Sinyal adı doğru olmalı
}

// onStopSimulationClicked slotu
void CerebrumLux::SimulationPanel::onStopSimulationClicked() {
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "SimulationPanel: Stop Simulation button clicked.");
    emit stopSimulationTriggered(); // Sinyal adı doğru olmalı
}

} // namespace CerebrumLux