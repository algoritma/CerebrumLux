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
#include "../gui/panels/QTablePanel.h" // YENİ: QTablePanel için başlık
#include "../gui/panels/ChatPanel.h"   // YENİ: ChatPanel için başlık
#include "../gui/engine_integration.h"
#include "../learning/Capsule.h"
#include "../communication/natural_language_processor.h" // generate_text_embedding için
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
    qTablePanel = new CerebrumLux::QTablePanel(learningModule, this); // YENİ: QTablePanel oluşturuldu
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MainWindow: QTablePanel oluşturuldu. Adresi: " << qTablePanel << ", isVisible(): " << qTablePanel->isVisible());
    chatPanel = new CerebrumLux::ChatPanel(this); // YENİ: ChatPanel oluşturuldu
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MainWindow: ChatPanel oluşturuldu. Adresi: " << chatPanel << ", isVisible(): " << chatPanel->isVisible());

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
    tabWidget->addTab(qTablePanel, "Q-Table"); // YENİ: Q-Table sekmesi eklendi
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MainWindow: Q-Table tab'ı eklendi. Tab sayısı: " << tabWidget->count());
    tabWidget->addTab(chatPanel, "Chat");      // YENİ: Chat sekmesi eklendi
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MainWindow: Chat tab'ı eklendi. Tab sayısı: " << tabWidget->count());

    setWindowTitle("Cerebrum Lux");
    resize(1024, 768);

    connect(logPanel, &CerebrumLux::LogPanel::logCleared, this, [this](){
        LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "GUI Log cleared by user.");
    });
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MainWindow: LogPanel connect tamamlandı.");

    connect(simulationPanel, &CerebrumLux::SimulationPanel::commandEntered, this, &CerebrumLux::MainWindow::onSimulationCommandEntered);
    connect(simulationPanel, &CerebrumLux::SimulationPanel::startSimulationTriggered, this, &CerebrumLux::MainWindow::onStartSimulationTriggered);
    connect(simulationPanel, &CerebrumLux::SimulationPanel::stopSimulationTriggered, this, &CerebrumLux::MainWindow::onStopSimulationTriggered);
    // DÜZELTİLDİ: Chat mesajı sinyali artık ChatPanel'den gelecek
    connect(chatPanel, &CerebrumLux::ChatPanel::chatMessageEntered, this, &CerebrumLux::MainWindow::onChatMessageReceived);
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MainWindow: SimulationPanel connect tamamlandı.");

    connect(capsuleTransferPanel, &CerebrumLux::CapsuleTransferPanel::ingestCapsuleRequest, this, &CerebrumLux::MainWindow::onIngestCapsuleRequest);
    connect(capsuleTransferPanel, &CerebrumLux::CapsuleTransferPanel::fetchWebCapsuleRequest, this, &CerebrumLux::MainWindow::onFetchWebCapsuleRequest);
     LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MainWindow: CapsuleTransferPanel connect tamamlandı.");

    connect(&learningModule, &CerebrumLux::LearningModule::webFetchCompleted, this, &CerebrumLux::MainWindow::onWebFetchCompleted);
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MainWindow: LearningModule::webFetchCompleted sinyali bağlandı.");

    // Düzeltme: QTablePanel'in LearningModule'den gelen sinyallere bağlanması.
    connect(&learningModule, &CerebrumLux::LearningModule::qTableUpdated, qTablePanel, &CerebrumLux::QTablePanel::updateQTableContent, Qt::QueuedConnection);
    connect(&learningModule, &CerebrumLux::LearningModule::qTableLoadCompleted, qTablePanel, &CerebrumLux::QTablePanel::updateQTableContent, Qt::QueuedConnection);

    // YENİ: KnowledgeBasePanel'i sinyale bağla
    connect(&learningModule, &CerebrumLux::LearningModule::knowledgeBaseUpdated, this, &CerebrumLux::MainWindow::updateKnowledgeBasePanel, Qt::QueuedConnection);

    guiUpdateTimer = new QTimer(this);
    connect(guiUpdateTimer, &QTimer::timeout, this, &CerebrumLux::MainWindow::updateGui);
    guiUpdateTimer->start(1000); // Düzeltme: Graph ve Simulation panellerinin güncellenmesi için zamanlayıcıyı yeniden başlat.
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MainWindow: GUI güncelleme zamanlayıcısı başlatıldı (1000ms). [Graph ve Simulation panellerini etkiler]");
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MainWindow: QTablePanel güncellemesi GUI zamanlayıcısına bağlandı.");

    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "MainWindow: Kurucu çıkışı. isVisible(): " << isVisible() << ", geometry: " << geometry().width() << "x" << geometry().height());
}

MainWindow::~MainWindow()
{
    guiUpdateTimer->stop();
    delete ui;
    // QTablePanel kendi workerThread'ini yönettiği için burada özel bir silme işlemi yapmaya gerek yok.
    // qTablePanel, MainWindow'ın bir child'ı olarak otomatik silinir.
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "MainWindow: Destructor called.");
}

LogPanel* MainWindow::getLogPanel() const {
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MainWindow: getLogPanel() çağrıldı. LogPanel adresi: " << logPanel);
    return logPanel;
}
GraphPanel* MainWindow::getGraphPanel() const { return graphPanel; }
SimulationPanel* MainWindow::getSimulationPanel() const { return simulationPanel; }
CapsuleTransferPanel* MainWindow::getCapsuleTransferPanel() const { return capsuleTransferPanel; }
QTablePanel* MainWindow::getQTablePanel() const { return qTablePanel; } // YENİ: QTablePanel getter implementasyonu
KnowledgeBasePanel* MainWindow::getKnowledgeBasePanel() const { return knowledgeBasePanel; }

void MainWindow::updateSimulationHistory(const QVector<CerebrumLux::SimulationData>& data) {
    if (simulationPanel) {
        simulationPanel->updateSimulationHistory(data);
    }
}

void MainWindow::updateGui() {
    // Statik NLP metodunu kullanarak embedding alıyoruz.
    // YENİ: Embedding'leri bir kez hesaplayıp önbelleğe alabiliriz, her updateGui() çağrısında tekrar hesaplamak yerine.
    static std::vector<float> sim_query_embedding = CerebrumLux::NaturalLanguageProcessor::generate_text_embedding("StepSimulation");
    std::vector<CerebrumLux::Capsule> capsules_for_sim = engine.getKnowledgeBase().semantic_search(sim_query_embedding, 100);
    QVector<CerebrumLux::SimulationData> sim_data;
    for (const auto& cap : capsules_for_sim) {
        sim_data.append(convertCapsuleToSimulationData(cap));
    }
    if (simulationPanel) {
        LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "MainWindow::updateGui: SimulationPanel güncelleniyor. Veri noktası sayısı: " << sim_data.size());
        simulationPanel->updateSimulationHistory(sim_data);
    } else {
        LOG_DEFAULT(CerebrumLux::LogLevel::WARNING, "MainWindow::updateGui: simulationPanel null. Simülasyon verisi güncellenemedi.");
    }


    // YENİ: GraphData embedding'ini bir kez hesaplayıp önbelleğe alıyoruz.
    static std::vector<float> graph_query_embedding = CerebrumLux::NaturalLanguageProcessor::generate_text_embedding("GraphData");
    auto capsules_for_graph = learningModule.getKnowledgeBase().semantic_search(graph_query_embedding, 100);
     QMap<qreal, qreal> graph_data;
    qreal min_confidence = std::numeric_limits<qreal>::max();
    qreal max_confidence = std::numeric_limits<qreal>::lowest();
    for (const auto& cap : capsules_for_graph) {
        /*
        // YENİ LOG: GraphData kapsüllerinin içeriğini kontrol et
        LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MainWindow::updateGui: GraphData Kapsül (ID: " << cap.id
                    << ", Topic: " << cap.topic << ", Confidence: " << cap.confidence
                    << ", Timestamp (ms): " << std::chrono::duration_cast<std::chrono::milliseconds>(cap.timestamp_utc.time_since_epoch()).count()
                    << ", Summary: " << cap.plain_text_summary.substr(0, std::min((size_t)50, cap.plain_text_summary.length())) << "...)");
        */
        graph_data.insert(std::chrono::duration_cast<std::chrono::milliseconds>(cap.timestamp_utc.time_since_epoch()).count(), static_cast<qreal>(cap.confidence));
        min_confidence = std::min(min_confidence, static_cast<qreal>(cap.confidence));
        max_confidence = std::max(max_confidence, static_cast<qreal>(cap.confidence));
    }
    if (graphPanel) {
        graphPanel->updateData("AI Confidence", graph_data);
        LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MainWindow::updateGui: GraphPanel güncellendi. Veri noktası sayısı: " << graph_data.size());
    } else {
        LOG_DEFAULT(CerebrumLux::LogLevel::WARNING, "MainWindow::updateGui: graphPanel null. Grafik verisi güncellenemedi.");
    }
    
    // KRİTİK PERFORMANS DÜZELTMESİ:
    // updateKnowledgeBasePanel() çağrısı buradan KALDIRILDI. 
    // Artık sadece sinyal geldiğinde (veri değiştiğinde) çalışacak.

    LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "MainWindow::updateGui: GUI güncellendi."); // TRACE seviyesindeki genel log korunuyor
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
    
    // DÜZELTME: Kullanıcı mesajını SequenceManager'a ekle, böylece NLP son mesajı okuyabilir.
    // Bu, generate_response çağrısından ÖNCE yapılmalıdır.
    engine.getSequenceManager().add_user_input(message.toStdString());
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MainWindow: Mesaj SequenceManager'a eklendi, NLP bekleniyor...");

    CerebrumLux::UserIntent user_intent = engine.getNlpProcessor().infer_intent_from_text(message.toStdString());
    CerebrumLux::AbstractState current_abstract_state = CerebrumLux::AbstractState::Idle;
    CerebrumLux::AIGoal current_goal = engine.getGoalManager().get_current_goal();
    
    // EngineIntegration'dan güncel sekansı alabiliriz
    const CerebrumLux::DynamicSequence& current_sequence = engine.getSequenceManager().get_current_sequence_ref();
    
    // Yanıt üretmek için NLP'yi kullan
    // DEĞİŞTİRİLEN KOD: generate_response'dan ChatResponse objesi al
    CerebrumLux::ChatResponse nlp_chat_response = engine.getResponseEngine().generate_response(user_intent, current_abstract_state, current_goal, current_sequence, learningModule.getKnowledgeBase());
    // Yanıtı GUI'ye ekle
    if (chatPanel) {
        chatPanel->appendChatMessage("CerebrumLux", nlp_chat_response); // Doğrudan ChatResponse objesini gönder
    } else {
        LOG_DEFAULT(CerebrumLux::LogLevel::WARNING, "MainWindow::onChatMessageReceived: chatPanel null. NLP yanıtı gösterilemedi.");
    }
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MainWindow: NLP yanıtı üretildi.");
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