#include "SimulationPanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout> 
#include <QLineEdit>   
#include <QPushButton> 
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QStringList>
#include "../../core/logger.h" 
#include <QDebug> // YENİ: qDebug için

SimulationPanel::SimulationPanel(QWidget* parent) : QWidget(parent)
{
    layout = new QVBoxLayout(this);

    // Simülasyon kontrol alanı (QLineEdit ve düğmeler)
    controlLayout = new QHBoxLayout();
    commandLineEdit = new QLineEdit(this);
    commandLineEdit->setPlaceholderText("Enter simulation command...");
    startButton = new QPushButton("Start Simulation", this);
    stopButton = new QPushButton("Stop Simulation", this);

    controlLayout->addWidget(commandLineEdit);
    controlLayout->addWidget(startButton);
    controlLayout->addWidget(stopButton);

    layout->addLayout(controlLayout); 

    table = new QTableWidget(this);
    table->setColumnCount(2);
    table->setHorizontalHeaderLabels(QStringList() << "Step" << "Value");
    layout->addWidget(table);
    setLayout(layout);

    // Sinyal-Slot bağlantıları
    // connect(commandLineEdit, &QLineEdit::returnPressed, this, &SimulationPanel::onCommandLineEditReturnPressed); // ESKİ BAĞLANTI (Yorum satırı)
    // YENİ TEŞHİS BAĞLANTISI: Alternatif sinyal veya string tabanlı bağlantı dene
    // Önce QLineEdit::returnPressed'in hala çalışıp çalışmadığını kontrol edelim.
    // Eğer bu işe yaramazsa, QLineEdit::editingFinished'i deneyebiliriz.

    // Qt 5 (ve 6) için önerilen yöntem:
    // connect(commandLineEdit, &QLineEdit::returnPressed, this, &SimulationPanel::onCommandLineEditReturnPressed);

    // Eğer yukarıdaki yöntem işe yaramıyorsa, string tabanlı eski yöntemi deneyelim (MOC sorunlarını atlatmak için)
    // Bu, MOC'un doğru çalışmadığı durumlarda yardımcı olabilir.
    bool connection_result = connect(commandLineEdit, SIGNAL(returnPressed()), this, SLOT(onCommandLineEditReturnPressed()));
    if (!connection_result) {
        qDebug() << "WARNING: QLineEdit::returnPressed connection failed in SimulationPanel constructor!";
    } else {
        qDebug() << "DEBUG: QLineEdit::returnPressed connected successfully in SimulationPanel constructor.";
    }

    connect(startButton, &QPushButton::clicked, this, &SimulationPanel::onStartSimulationClicked);
    connect(stopButton, &QPushButton::clicked, this, &SimulationPanel::onStopSimulationClicked);

    LOG_DEFAULT(LogLevel::INFO, "SimulationPanel: Initialized with input and control buttons.");
    qDebug() << "DEBUG: SimulationPanel constructor finished.";
}

void SimulationPanel::updatePanel(const std::vector<SimulationData>& data)
{
    table->setRowCount(static_cast<int>(data.size()));
    for (size_t i = 0; i < data.size(); ++i) {
        table->setItem(i, 0, new QTableWidgetItem(QString::number(data[i].id))); 
        table->setItem(i, 1, new QTableWidgetItem(QString::number(data[i].value)));
    }
}

// Slot implementasyonları
void SimulationPanel::onCommandLineEditReturnPressed() {
    qDebug() << "DEBUG: SimulationPanel::onCommandLineEditReturnPressed slot triggered. (via qDebug)"; // YENİ TEŞHİS LOGU
    LOG_DEFAULT(LogLevel::DEBUG, "SimulationPanel: onCommandLineEditReturnPressed slot triggered."); 
    QString command = commandLineEdit->text();
    qDebug() << "DEBUG: SimulationPanel: CommandLineEdit text: '" << command << "' (isEmpty: " << command.isEmpty() << ")"; // YENİ TEŞHİS LOGU
    LOG_DEFAULT(LogLevel::DEBUG, "SimulationPanel: CommandLineEdit text: '" << command.toStdString() << "' (isEmpty: " << command.isEmpty() << ")"); 

    if (!command.isEmpty()) {
        LOG_DEFAULT(LogLevel::INFO, "SimulationPanel: Command entered: " << command.toStdString());
        emit commandEntered(command); 
        commandLineEdit->clear(); 
    } else {
        LOG_DEFAULT(LogLevel::WARNING, "SimulationPanel: Command entered was empty."); 
    }
}

void SimulationPanel::onStartSimulationClicked() {
    qDebug() << "DEBUG: SimulationPanel: Start Simulation button clicked. (via qDebug)";
    LOG_DEFAULT(LogLevel::INFO, "SimulationPanel: Start Simulation button clicked.");
    emit startSimulation(); 
}

void SimulationPanel::onStopSimulationClicked() {
    qDebug() << "DEBUG: SimulationPanel: Stop Simulation button clicked. (via qDebug)";
    LOG_DEFAULT(LogLevel::INFO, "SimulationPanel: Stop Simulation button clicked.");
    emit stopSimulation(); 
}