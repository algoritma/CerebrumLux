#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QTimer>
#include <QDateTime>
#include <QInputDialog>
#include <QMessageBox>
#include <QDebug>

// Kendi namespace'imizdeki sınıfları doğrudan kullanabilmek için
#include "../core/logger.h"
#include "../gui/panels/LogPanel.h" // YENİ EKLENDİ (MainWindow.cpp için)
#include "../gui/panels/GraphPanel.h" // YENİ EKLENDİ (MainWindow.cpp için)
#include "../gui/panels/SimulationPanel.h"
#include "../gui/panels/CapsuleTransferPanel.h"
#include "../gui/engine_integration.h"
#include "../learning/Capsule.h"
#include "../learning/LearningModule.h" // CerebrumLux::IngestReport, IngestResult için tam tanım

#include "../external/nlohmann/json.hpp" // JSON parsing için

namespace CerebrumLux {

SimulationData MainWindow::convertCapsuleToSimulationData(const Capsule& capsule) {
    SimulationData data;
    data.id = QString::fromStdString(capsule.id);
    data.value = capsule.confidence;
    data.timestamp = std::chrono::system_clock::to_time_t(capsule.timestamp_utc);
    return data;
}

MainWindow::MainWindow(EngineIntegration& engineRef, LearningModule& learningModuleRef, QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow), // ui_MainWindow.h dahil edildiği için tam tanıma sahibiz
      engine(engineRef),
      learningModule(learningModuleRef)
{
    ui->setupUi(this);

    qRegisterMetaType<CerebrumLux::IngestResult>("CerebrumLux::IngestResult");
    qRegisterMetaType<CerebrumLux::IngestReport>("CerebrumLux::IngestReport");

    // UI bileşenlerini oluşturma
    tabWidget = new QTabWidget(this);
    setCentralWidget(tabWidget);

    // Panel sınıfları artık tam olarak bilindiği için 'new' çağrısı geçerli
    logPanel = new CerebrumLux::LogPanel(this); // Namespace ile
    graphPanel = new CerebrumLux::GraphPanel(this); // Namespace ile
    simulationPanel = new CerebrumLux::SimulationPanel(this); // Namespace ile
    capsuleTransferPanel = new CerebrumLux::CapsuleTransferPanel(this); // Namespace ile

    tabWidget->addTab(logPanel, "Log");
    tabWidget->addTab(graphPanel, "Graph");
    tabWidget->addTab(simulationPanel, "Simulation");
    tabWidget->addTab(capsuleTransferPanel, "Capsule Transfer");

    // Sinyal/slot bağlantıları
    connect(logPanel, &CerebrumLux::LogPanel::logCleared, this, [this](){ // Namespace ile
        LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "GUI Log cleared by user.");
    });
    connect(simulationPanel, &CerebrumLux::SimulationPanel::commandEntered, this, &CerebrumLux::MainWindow::onSimulationCommandEntered); // Namespace ile
    connect(simulationPanel, &CerebrumLux::SimulationPanel::startSimulationTriggered, this, &CerebrumLux::MainWindow::onStartSimulationTriggered); // Namespace ile
    connect(simulationPanel, &CerebrumLux::SimulationPanel::stopSimulationTriggered, this, &CerebrumLux::MainWindow::onStopSimulationTriggered); // Namespace ile

    connect(capsuleTransferPanel, &CerebrumLux::CapsuleTransferPanel::ingestCapsuleRequest, this, &CerebrumLux::MainWindow::onIngestCapsuleRequest); // Namespace ile
    connect(capsuleTransferPanel, &CerebrumLux::CapsuleTransferPanel::fetchWebCapsuleRequest, this, &CerebrumLux::MainWindow::onFetchWebCapsuleRequest); // Namespace ile

    // GUI güncelleme zamanlayıcısı
    guiUpdateTimer = new QTimer(this);
    connect(guiUpdateTimer, &QTimer::timeout, this, &CerebrumLux::MainWindow::updateGui); // Namespace ile
    guiUpdateTimer->start(100); // Her 100 ms'de bir GUI'yi güncelle

    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "MainWindow: GUI Initialized.");
}

MainWindow::~MainWindow()
{
    guiUpdateTimer->stop();
    delete ui; // Ui::MainWindow artık tam olarak tanımlı olduğu için delete güvenli
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "MainWindow: Destructor called.");
}

LogPanel* MainWindow::getLogPanel() const { return logPanel; }
GraphPanel* MainWindow::getGraphPanel() const { return graphPanel; }
SimulationPanel* MainWindow::getSimulationPanel() const { return simulationPanel; }
CapsuleTransferPanel* MainWindow::getCapsuleTransferPanel() const { return capsuleTransferPanel; }

void MainWindow::appendLog(CerebrumLux::LogLevel level, const QString& message) {
    if (logPanel) {
        logPanel->appendLog(level, message);
    }
}

void MainWindow::updateGraphData(const QString& seriesName, const QMap<qreal, qreal>& data) {
    if (graphPanel) {
        graphPanel->updateData(seriesName, data);
    }
}

void MainWindow::updateSimulationHistory(const QVector<CerebrumLux::SimulationData>& data) {
    if (simulationPanel) {
        simulationPanel->updateSimulationHistory(data);
    }
}

void MainWindow::updateGui() {
    // KnowledgeBase'den simülasyon verilerini al
    std::vector<CerebrumLux::Capsule> capsules_for_sim = engine.getKnowledgeBase().search_by_topic("StepSimulation");
    QVector<CerebrumLux::SimulationData> sim_data;
    for (const auto& cap : capsules_for_sim) {
        sim_data.append(convertCapsuleToSimulationData(cap));
    }
    simulationPanel->updateSimulationHistory(sim_data);

    // KnowledgeBase'den grafik verilerini al
    auto capsules_for_graph = learningModule.search_by_topic("StepSimulation");
    QMap<qreal, qreal> graph_data;
    for (const auto& cap : capsules_for_graph) {
        graph_data.insert(std::chrono::duration_cast<std::chrono::milliseconds>(cap.timestamp_utc.time_since_epoch()).count(), cap.confidence);
    }
    graphPanel->updateData("Confidence Over Time", graph_data);

    auto results = learningModule.getKnowledgeBase().semantic_search("Qt6", 2);
    if (!results.empty()) {
        //LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "Semantic search results: " << results[0].content.substr(0, 50));
    }
}

void MainWindow::onSimulationCommandEntered(const QString& command) {
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "Simulation Command: " << command.toStdString());
    engine.processUserCommand(command.toStdString());
}

void MainWindow::onStartSimulationTriggered() {
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "Start Simulation Triggered.");
    engine.startCoreSimulation();
}

void MainWindow::onStopSimulationTriggered() {
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "Stop Simulation Triggered.");
    engine.stopCoreSimulation();
}

void MainWindow::onIngestCapsuleRequest(const QString& capsuleJson, const QString& signature, const QString& senderId) {
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "MainWindow: Ingest Capsule Request from " << senderId.toStdString());
    try {
        nlohmann::json j = nlohmann::json::parse(capsuleJson.toStdString());
        CerebrumLux::Capsule incoming_capsule = j.get<CerebrumLux::Capsule>();

        CerebrumLux::IngestReport report = learningModule.ingest_envelope(incoming_capsule, signature.toStdString(), senderId.toStdString());
        capsuleTransferPanel->displayIngestReport(report);
    } catch (const nlohmann::json::parse_error& e) {
        LOG_DEFAULT(CerebrumLux::LogLevel::ERR_CRITICAL, "MainWindow: JSON Parse Error on ingest request: " << e.what());
        CerebrumLux::IngestReport error_report;
        error_report.result = CerebrumLux::IngestResult::SchemaMismatch;
        error_report.message = "Invalid JSON format: " + std::string(e.what());
        capsuleTransferPanel->displayIngestReport(error_report);
    } catch (const std::exception& e) {
        LOG_DEFAULT(CerebrumLux::LogLevel::ERR_CRITICAL, "MainWindow: Error ingesting capsule: " << e.what());
        CerebrumLux::IngestReport error_report;
        error_report.result = CerebrumLux::IngestResult::UnknownError;
        error_report.message = "An unknown error occurred: " + std::string(e.what());
        capsuleTransferPanel->displayIngestReport(error_report);
    }
}

void MainWindow::onFetchWebCapsuleRequest(const QString& query) {
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "MainWindow: Fetch Web Capsule Request for query: " << query.toStdString());
    learningModule.learnFromWeb(query.toStdString());

    CerebrumLux::IngestReport dummy_report;
    dummy_report.result = CerebrumLux::IngestResult::Success;
    dummy_report.message = "Web capsule fetch initiated for: " + query.toStdString();
    capsuleTransferPanel->displayIngestReport(dummy_report);
}

} // namespace CerebrumLux