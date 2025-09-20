#include "MainWindow.h"
#include <QTimer>
#include <iostream>
#include <QStringList> 
#include <QDebug> 
#include "../core/logger.h" 
#include "../learning/capsule.h" 

// Panel başlık dosyalarını burada dahil ediyoruz, çünkü tanımlamalarına burada ihtiyacımız var.
#include "panels/SimulationPanel.h"
#include "panels/LogPanel.h" 
#include "panels/GraphPanel.h" 
#include "../gui/engine_integration.h" // YENİ: EngineIntegration için include


// Capsule'dan SimulationData'ya basit bir dönüştürücü (gerekirse DataTypes.h'ye taşınabilir)
SimulationData convertCapsuleToSimulationData(const Capsule& capsule) {
    SimulationData data;
    data.id = capsule.id; 
    data.value = capsule.confidence; 
    return data;
}


// Constructor
MainWindow::MainWindow(EngineIntegration& eng, LearningModule& learn, QWidget* parent)
    : QMainWindow(parent), engine(eng), learningModule(learn)
{
    // Paneller
    tabWidget = new QTabWidget(this);
    simulationPanel = new SimulationPanel(this);
    logPanel = new LogPanel(this);
    graphPanel = new GraphPanel(this);

    tabWidget->addTab(simulationPanel, "Simulation");
    tabWidget->addTab(logPanel, "Log");
    tabWidget->addTab(graphPanel, "Graph");

    setCentralWidget(tabWidget);

    // GUI güncelleme timer
    connect(&update_timer, &QTimer::timeout, this, &MainWindow::updateGui);
    update_timer.start(500); 

    // SimulationPanel sinyallerini MainWindow'daki slot'lara bağla
    connect(simulationPanel, &SimulationPanel::commandEntered, this, &MainWindow::onSimulationCommandEntered);
    connect(simulationPanel, &SimulationPanel::startSimulation, this, &MainWindow::onStartSimulationTriggered);
    connect(simulationPanel, &SimulationPanel::stopSimulation, this, &MainWindow::onStopSimulationTriggered);

    LOG(LogLevel::INFO, "MainWindow: SimulationPanel signals connected.");
}

// Yıkıcı
MainWindow::~MainWindow() {
    // Paneller QObject hiyerarşisi tarafından otomatik yönetilir.
}

// GUI güncelleme metodu
void MainWindow::updateGui() {
    std::vector<Capsule> capsules_for_sim = engine.getKnowledgeBase().getCapsulesByTopic("StepSimulation");
    std::vector<SimulationData> simulation_data_vec;
    for (const auto& cap : capsules_for_sim) {
        simulation_data_vec.push_back(convertCapsuleToSimulationData(cap)); 
    }
    simulationPanel->updatePanel(simulation_data_vec);

    auto capsules_for_graph = learningModule.getCapsulesByTopic("StepSimulation");
    if (!capsules_for_graph.empty()) {
        double latestValue = capsules_for_graph.back().confidence; 
        graphPanel->updateGraph(static_cast<size_t>(latestValue));
        LOG_DEFAULT(LogLevel::DEBUG, "MainWindow: GraphPanel updated with value: " << latestValue);
    } else {
        LOG_DEFAULT(LogLevel::DEBUG, "MainWindow: No 'StepSimulation' capsules found for GraphPanel. Setting value to 0.");
        graphPanel->updateGraph(0); 
    }

    LOG_DEFAULT(LogLevel::DEBUG, "[GUI] Güncelleme..."); 
    qDebug() << "[Qt GUI] Güncelleme..."; 

    auto results = learningModule.getKnowledgeBase().findSimilar("Qt6", 2);
    if (!results.empty()) {
        LOG_DEFAULT(LogLevel::INFO, "[GUI] Hatırlanan bilgi: " << results[0].content);
        qDebug() << "[Qt GUI] Hatırlanan bilgi: " << QString::fromStdString(results[0].content);
    }
}

// SimulationPanel sinyalleri için slot implementasyonları
void MainWindow::onSimulationCommandEntered(const QString& command) {
    LOG(LogLevel::INFO, "MainWindow: Received simulation command: " << command.toStdString());
    // YENİ: EngineIntegration'a komutu ilet
    engine.processUserCommand(command.toStdString());
}

void MainWindow::onStartSimulationTriggered() {
    LOG(LogLevel::INFO, "MainWindow: Received start simulation signal.");
    // YENİ: EngineIntegration üzerinden simülasyonu başlat
    engine.startCoreSimulation();
}

void MainWindow::onStopSimulationTriggered() {
    LOG(LogLevel::INFO, "MainWindow: Received stop simulation signal.");
    // YENİ: EngineIntegration üzerinden simülasyonu durdur
    engine.stopCoreSimulation();
}