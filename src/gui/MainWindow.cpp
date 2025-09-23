#include "MainWindow.h"
#include <QTimer>
#include <iostream>
#include <QStringList> 
#include <QDebug> 
#include "../core/logger.h" 
#include "../learning/capsule.h" 
#include <iomanip> 
#include <sstream> 
#include <nlohmann/json.hpp> // JSON ayrıştırma için

// OpenSSL için gerekli başlıklar (Sadece EVP_CIPHER_iv_length için gerekli olanı bırakıldı)
#include <openssl/evp.h>    

// Panel başlık dosyalarını burada dahil ediyoruz
#include "panels/SimulationPanel.h"
#include "panels/LogPanel.h" 
#include "panels/GraphPanel.h" 
#include "panels/CapsuleTransferPanel.h" // YENİ: CapsuleTransferPanel dahil edildi
#include "../gui/engine_integration.h" 
#include "../core/utils.h" // base64_encode için utils.h'yi de dahil edelim

// Capsule'dan SimulationData'ya basit bir dönüştürücü
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
    // Q_DECLARE_METATYPE ile kaydedilen türleri sinyal/slot sisteminde kullanabilmek için
    // qRegisterMetaType çağrısı burada yapılabilir.
    qRegisterMetaType<IngestResult>("IngestResult");
    qRegisterMetaType<IngestReport>("IngestReport");

    // Paneller
    tabWidget = new QTabWidget(this);
    simulationPanel = new SimulationPanel(this);
    logPanel = new LogPanel(this);
    graphPanel = new GraphPanel(this);
    capsuleTransferPanel = new CapsuleTransferPanel(this); // YENİ: CapsuleTransferPanel oluşturuldu

    tabWidget->addTab(simulationPanel, "Simulation");
    tabWidget->addTab(logPanel, "Log");
    tabWidget->addTab(graphPanel, "Graph");
    tabWidget->addTab(capsuleTransferPanel, "Capsule Transfer"); // YENİ: Panele eklendi

    setCentralWidget(tabWidget);

    // GUI güncelleme timer
    connect(&update_timer, &QTimer::timeout, this, &MainWindow::updateGui);
    update_timer.start(500); 

    // SimulationPanel sinyallerini MainWindow'daki slot'lara bağla
    connect(simulationPanel, &SimulationPanel::commandEntered, this, &MainWindow::onSimulationCommandEntered);
    // Düzeltilmiş bağlantılar: SimulationPanel'in kendi sinyallerine bağlanıyoruz
    connect(simulationPanel, &SimulationPanel::startSimulation, this, &MainWindow::onStartSimulationTriggered);
    connect(simulationPanel, &SimulationPanel::stopSimulation, this, &MainWindow::onStopSimulationTriggered);

    // YENİ: CapsuleTransferPanel sinyallerini MainWindow'daki slot'lara bağla
    connect(capsuleTransferPanel, &CapsuleTransferPanel::ingestCapsuleRequest, this, &MainWindow::onIngestCapsuleRequest);
    connect(capsuleTransferPanel, &CapsuleTransferPanel::fetchWebCapsuleRequest, this, &MainWindow::onFetchWebCapsuleRequest);

    LOG(LogLevel::INFO, "MainWindow: SimulationPanel and CapsuleTransferPanel signals connected.");
}

// Yıkıcı
MainWindow::~MainWindow() {
    // Paneller QObject hiyerarşisi tarafından otomatik yönetilir.
}

// GUI güncelleme metodu
void MainWindow::updateGui() {
    std::vector<Capsule> capsules_for_sim = engine.getKnowledgeBase().search_by_topic("StepSimulation"); 
    std::vector<SimulationData> simulation_data_vec;
    for (const auto& cap : capsules_for_sim) {
        simulation_data_vec.push_back(convertCapsuleToSimulationData(cap)); 
    }
    simulationPanel->updatePanel(simulation_data_vec);

    auto capsules_for_graph = learningModule.search_by_topic("StepSimulation"); 
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

    auto results = learningModule.getKnowledgeBase().semantic_search("Qt6", 2); 
    if (!results.empty()) {
        LOG_DEFAULT(LogLevel::INFO, "[GUI] Hatırlanan bilgi: " << results[0].content);
        qDebug() << "[Qt GUI] Hatırlanan bilgi: " << QString::fromStdString(results[0].content);
    }
}

// SimulationPanel sinyalleri için slot implementasyonları
void MainWindow::onSimulationCommandEntered(const QString& command) {
    LOG(LogLevel::INFO, "MainWindow: Received simulation command: " << command.toStdString());
    engine.processUserCommand(command.toStdString());
}

void MainWindow::onStartSimulationTriggered() {
    LOG(LogLevel::INFO, "MainWindow: Received start simulation signal.");
    engine.startCoreSimulation();
}

void MainWindow::onStopSimulationTriggered() {
    LOG(LogLevel::INFO, "MainWindow: Received stop simulation signal.");
    engine.stopCoreSimulation();
}

// YENİ: CapsuleTransferPanel sinyalleri için slot implementasyonları
void MainWindow::onIngestCapsuleRequest(const QString& capsuleJson, const QString& signature, const QString& senderId) {
    LOG_DEFAULT(LogLevel::INFO, "MainWindow: Received ingest capsule request from GUI. Sender: " << senderId.toStdString());
    
    try {
        nlohmann::json j = nlohmann::json::parse(capsuleJson.toStdString());
        Capsule incoming_capsule = Capsule::fromJson(j);

        // LearningModule'den kapsülü işle
        IngestReport report = learningModule.ingest_envelope(incoming_capsule, signature.toStdString(), senderId.toStdString());
        
        // Raporu CapsuleTransferPanel'e geri gönder
        capsuleTransferPanel->displayIngestReport(report);

    } catch (const nlohmann::json::parse_error& e) {
        LOG_DEFAULT(LogLevel::ERR_CRITICAL, "MainWindow: JSON ayrıştırma hatası: " << e.what());
        IngestReport error_report;
        error_report.result = IngestResult::UnknownError;
        error_report.message = "JSON parsing failed: " + std::string(e.what());
        error_report.source_peer_id = senderId.toStdString();
        error_report.timestamp = std::chrono::system_clock::now();
        capsuleTransferPanel->displayIngestReport(error_report);
    } catch (const std::exception& e) {
        LOG_DEFAULT(LogLevel::ERR_CRITICAL, "MainWindow: Kapsül yutma sırasında beklenmeyen hata: " << e.what());
        IngestReport error_report;
        error_report.result = IngestResult::UnknownError;
        error_report.message = "Unexpected error during capsule ingestion: " + std::string(e.what());
        error_report.source_peer_id = senderId.toStdString();
        error_report.timestamp = std::chrono::system_clock::now();
        capsuleTransferPanel->displayIngestReport(error_report);
    }
}

void MainWindow::onFetchWebCapsuleRequest(const QString& query) {
    LOG_DEFAULT(LogLevel::INFO, "MainWindow: Received fetch web capsule request from GUI. Query: " << query.toStdString());
    // LearningModule'den web'den kapsül çekme işlemini başlat
    learningModule.learnFromWeb(query.toStdString());

    // Başarılı olduğunu varsayan basit bir rapor (gerçekte learnFromWeb bir rapor döndürmez)
    IngestReport dummy_report;
    dummy_report.result = IngestResult::Success;
    dummy_report.message = "Web'den kapsül çekme işlemi başlatıldı (simüle edildi).";
    dummy_report.source_peer_id = "WebFetcher";
    dummy_report.original_capsule.id = "web_fetch_" + query.toStdString().substr(0, std::min(static_cast<size_t>(10), static_cast<size_t>(query.length())));
    dummy_report.timestamp = std::chrono::system_clock::now();
    capsuleTransferPanel->displayIngestReport(dummy_report);
}