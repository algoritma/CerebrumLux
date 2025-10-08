#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QTimer>
#include <QDateTime>
#include <QInputDialog>
#include <QMessageBox>
#include <QDebug>

#include "../core/logger.h"
#include "../gui/panels/LogPanel.h"
#include "../gui/panels/GraphPanel.h"
#include "../gui/panels/SimulationPanel.h"
#include "../gui/panels/CapsuleTransferPanel.h"
#include "../gui/panels/KnowledgeBasePanel.h"
#include "../gui/engine_integration.h"
#include "../learning/Capsule.h"
#include "../learning/LearningModule.h"

#include "../external/nlohmann/json.hpp"

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
      ui(new Ui::MainWindow),
      engine(engineRef),
      learningModule(learningModuleRef)
{
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MainWindow: Kurucuya girildi.");
    
    ui->setupUi(this);
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MainWindow: ui->setupUi(this) çağrıldı.");

    tabWidget = new QTabWidget(this);
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MainWindow: QTabWidget oluşturuldu. Adresi: " << tabWidget);
    setCentralWidget(tabWidget);
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MainWindow: QTabWidget merkezi widget olarak ayarlandı. isVisible(): " << tabWidget->isVisible());

    logPanel = new CerebrumLux::LogPanel(this);
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MainWindow: LogPanel oluşturuldu. Adresi: " << logPanel << ", isVisible(): " << logPanel->isVisible());
    graphPanel = new CerebrumLux::GraphPanel(this);
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MainWindow: GraphPanel oluşturuldu. Adresi: " << graphPanel << ", isVisible(): " << graphPanel->isVisible());
    simulationPanel = new CerebrumLux::SimulationPanel(this);
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MainWindow: SimulationPanel oluşturuldu. Adresi: " << simulationPanel << ", isVisible(): " << simulationPanel->isVisible());
    capsuleTransferPanel = new CerebrumLux::CapsuleTransferPanel(this);
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MainWindow: CapsuleTransferPanel oluşturuldu. Adresi: " << capsuleTransferPanel << ", isVisible(): " << capsuleTransferPanel->isVisible());
    knowledgeBasePanel = new CerebrumLux::KnowledgeBasePanel(learningModule, this);
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MainWindow: KnowledgeBasePanel oluşturuldu. Adresi: " << knowledgeBasePanel << ", isVisible(): " << knowledgeBasePanel->isVisible());


    tabWidget->addTab(logPanel, "Log");
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MainWindow: Log tab'ı eklendi. Aktif widget: " << tabWidget->currentWidget());
    tabWidget->addTab(graphPanel, "Graph");
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MainWindow: Graph tab'ı eklendi.");
    tabWidget->addTab(simulationPanel, "Simulation");
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MainWindow: Simulation tab'ı eklendi.");
    tabWidget->addTab(capsuleTransferPanel, "Capsule Transfer");
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MainWindow: Capsule Transfer tab'ı eklendi.");
    tabWidget->addTab(knowledgeBasePanel, "KnowledgeBase");
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MainWindow: KnowledgeBase tab'ı eklendi. Tab sayısı: " << tabWidget->count());


    setWindowTitle("Cerebrum Lux");
    resize(1024, 768);

    connect(logPanel, &CerebrumLux::LogPanel::logCleared, this, [this](){
        LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "GUI Log cleared by user.");
    });
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MainWindow: LogPanel connect tamamlandı.");

    connect(simulationPanel, &CerebrumLux::SimulationPanel::commandEntered, this, &CerebrumLux::MainWindow::onSimulationCommandEntered);
    connect(simulationPanel, &CerebrumLux::SimulationPanel::startSimulationTriggered, this, &CerebrumLux::MainWindow::onStartSimulationTriggered);
    connect(simulationPanel, &CerebrumLux::SimulationPanel::stopSimulationTriggered, this, &CerebrumLux::MainWindow::onStopSimulationTriggered);
    // YENİ: SimulationPanel'den gelen chat mesajı sinyalini bağla
    connect(simulationPanel, &CerebrumLux::SimulationPanel::chatMessageEntered, this, &CerebrumLux::MainWindow::onChatMessageReceived);
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MainWindow: SimulationPanel connect tamamlandı.");

    connect(capsuleTransferPanel, &CerebrumLux::CapsuleTransferPanel::ingestCapsuleRequest, this, &CerebrumLux::MainWindow::onIngestCapsuleRequest);
    connect(capsuleTransferPanel, &CerebrumLux::CapsuleTransferPanel::fetchWebCapsuleRequest, this, &CerebrumLux::MainWindow::onFetchWebCapsuleRequest);
     LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MainWindow: CapsuleTransferPanel connect tamamlandı.");

    connect(&learningModule, &CerebrumLux::LearningModule::webFetchCompleted, this, &CerebrumLux::MainWindow::onWebFetchCompleted);
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MainWindow: LearningModule::webFetchCompleted sinyali bağlandı.");


    guiUpdateTimer = new QTimer(this);
    connect(guiUpdateTimer, &QTimer::timeout, this, &CerebrumLux::MainWindow::updateGui);
    guiUpdateTimer->start(1000);
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MainWindow: GUI güncelleme zamanlayıcısı başlatıldı (1000ms).");

    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "MainWindow: Kurucu çıkışı. isVisible(): " << isVisible() << ", geometry: " << geometry().width() << "x" << geometry().height());
}

MainWindow::~MainWindow()
{
    guiUpdateTimer->stop();
    delete ui;
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "MainWindow: Destructor called.");
}

LogPanel* MainWindow::getLogPanel() const {
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MainWindow: getLogPanel() çağrıldı. LogPanel adresi: " << logPanel);
    return logPanel;
}
GraphPanel* MainWindow::getGraphPanel() const { return graphPanel; }
SimulationPanel* MainWindow::getSimulationPanel() const { return simulationPanel; }
CapsuleTransferPanel* MainWindow::getCapsuleTransferPanel() const { return capsuleTransferPanel; }
KnowledgeBasePanel* MainWindow::getKnowledgeBasePanel() const { return knowledgeBasePanel; }

 

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
    std::vector<CerebrumLux::Capsule> capsules_for_sim = engine.getKnowledgeBase().semantic_search("StepSimulation", 100);
    QVector<CerebrumLux::SimulationData> sim_data;
    for (const auto& cap : capsules_for_sim) {
        sim_data.append(convertCapsuleToSimulationData(cap));
    }
    if (simulationPanel) {
        simulationPanel->updateSimulationHistory(sim_data);
    } else {
        LOG_DEFAULT(CerebrumLux::LogLevel::WARNING, "MainWindow::updateGui: simulationPanel null. Simülasyon verisi güncellenemedi.");
    }


    auto capsules_for_graph = learningModule.getKnowledgeBase().semantic_search("GraphData", 100);
    QMap<qreal, qreal> graph_data;
    for (const auto& cap : capsules_for_graph) {
        graph_data.insert(std::chrono::duration_cast<std::chrono::milliseconds>(cap.timestamp_utc.time_since_epoch()).count(), cap.confidence);
    }
    if (graphPanel) {
        graphPanel->updateData("AI Confidence", graph_data);
        LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MainWindow::updateGui: GraphPanel güncellendi. Veri noktası sayısı: " << graph_data.size());
    } else {
        LOG_DEFAULT(CerebrumLux::LogLevel::WARNING, "MainWindow::updateGui: graphPanel null. Grafik verisi güncellenemedi.");
    }
    
    updateKnowledgeBasePanel();
    
    auto results = learningModule.getKnowledgeBase().semantic_search("Qt6", 2);
    if (!results.empty()) {
        LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MainWindow::updateGui: Semantic search results: " << results[0].content.substr(0, std::min((size_t)50, results[0].content.length())));
    }

    LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "MainWindow::updateGui: GUI güncellendi.");
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
        if (capsuleTransferPanel) {
            capsuleTransferPanel->displayIngestReport(report);
        } else {
            LOG_DEFAULT(CerebrumLux::LogLevel::WARNING, "MainWindow::onIngestCapsuleRequest: capsuleTransferPanel null. Ingest raporu gösterilemedi.");
        }
    } catch (const nlohmann::json::parse_error& e) {
        LOG_DEFAULT(CerebrumLux::LogLevel::ERR_CRITICAL, "MainWindow: JSON Parse Error on ingest request: " << e.what());
        CerebrumLux::IngestReport error_report;
        error_report.result = CerebrumLux::IngestResult::SchemaMismatch;
        error_report.message = "Invalid JSON format: " + std::string(e.what());
        if (capsuleTransferPanel) {
            capsuleTransferPanel->displayIngestReport(error_report);
        }
    } catch (const std::exception& e) {
        LOG_DEFAULT(CerebrumLux::LogLevel::ERR_CRITICAL, "MainWindow: Error ingesting capsule: " << e.what());
        CerebrumLux::IngestReport error_report;
        error_report.result = CerebrumLux::IngestResult::UnknownError;
        error_report.message = "An unknown error occurred: " + std::string(e.what());
        if (capsuleTransferPanel) {
            capsuleTransferPanel->displayIngestReport(error_report);
        }
    }
}

void MainWindow::onFetchWebCapsuleRequest(const QString& query) {
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "MainWindow: Fetch Web Capsule Request for query: " << query.toStdString());
    learningModule.learnFromWeb(query.toStdString());
}

void MainWindow::onWebFetchCompleted(const CerebrumLux::IngestReport& report) {
    if (capsuleTransferPanel) {
        capsuleTransferPanel->displayIngestReport(report);
        LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "MainWindow: Web çekme raporu CapsuleTransferPanel'de gösterildi. Sonuç: " << static_cast<int>(report.result));
    } else {
        LOG_DEFAULT(CerebrumLux::LogLevel::WARNING, "MainWindow::onWebFetchCompleted: capsuleTransferPanel null. Web çekme raporu gösterilemedi.");
    }
}

void MainWindow::onChatMessageReceived(const QString& message) {
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "MainWindow: Chat mesajı alındı: " << message.toStdString());
    
    CerebrumLux::UserIntent user_intent = engine.getNlpProcessor().infer_intent_from_text(message.toStdString());
    CerebrumLux::AbstractState current_abstract_state = CerebrumLux::AbstractState::Idle;// Şimdilik varsayılan
    CerebrumLux::AIGoal current_goal = engine.getGoalManager().get_current_goal();
    
    // EngineIntegration'dan güncel sekansı alabiliriz
    const CerebrumLux::DynamicSequence& current_sequence = engine.getSequenceManager().get_current_sequence_ref();
    
    // Yanıt üretmek için NLP'yi kullan
    // DEĞİŞTİRİLEN KOD: generate_response'dan ChatResponse objesi al
    CerebrumLux::ChatResponse nlp_chat_response = engine.getResponseEngine().generate_response(user_intent, current_abstract_state, current_goal, current_sequence, learningModule.getKnowledgeBase());

    // DİKKAT: SimulationPanel'in appendChatMessage metodunu da ChatResponse alacak şekilde güncellemeniz gerekebilir.
    // Şimdilik sadece text kısmını gönderiyoruz.

    // Yanıtı SimulationPanel'e geri gönder
    if (simulationPanel) {
        simulationPanel->appendChatMessage("CerebrumLux", nlp_chat_response); // DEĞİŞTİRİLDİ: Doğrudan ChatResponse objesini gönder
        // İleride, nlp_chat_response.reasoning ve nlp_chat_response.needs_clarification da burada görüntülenebilir.
    } else {
        LOG_DEFAULT(CerebrumLux::LogLevel::WARNING, "MainWindow::onChatMessageReceived: simulationPanel null. NLP yanıtı gösterilemedi.");
    }
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MainWindow: NLP yanıtı üretildi (metin): " << nlp_chat_response.text.substr(0, std::min((size_t)50, nlp_chat_response.text.length()))
        << ", Gerekçe: " << nlp_chat_response.reasoning.substr(0, std::min((size_t)50, nlp_chat_response.reasoning.length()))
        << ", Açıklama Gerekli: " << (nlp_chat_response.needs_clarification ? "Evet" : "Hayır"));
}

void MainWindow::updateKnowledgeBasePanel() {
    if (knowledgeBasePanel) {
        knowledgeBasePanel->updateKnowledgeBaseContent();
        LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MainWindow::updateKnowledgeBasePanel: KnowledgeBasePanel güncellendi.");
    } else {
        LOG_DEFAULT(CerebrumLux::LogLevel::WARNING, "MainWindow::updateKnowledgeBasePanel: knowledgeBasePanel null. KnowledgeBase listesi güncellenemedi.");
    }
}

} // namespace CerebrumLux