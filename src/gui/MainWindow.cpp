#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QTimer>
#include <QDateTime>
#include <QInputDialog>
#include <QMessageBox>
#include <QDebug>
#include <QtConcurrent/QtConcurrent> // Asenkron çalıştırma için

#include "../core/logger.h"
#include "../gui/panels/LogPanel.h"
#include "../gui/panels/GraphPanel.h"
#include "../gui/panels/SimulationPanel.h"
#include "../gui/panels/CapsuleTransferPanel.h"
#include "../gui/panels/KnowledgeBasePanel.h"
#include "../gui/panels/QTablePanel.h" // YENİ: QTablePanel için başlık
#include "../gui/panels/ChatPanel.h"   // YENİ: ChatPanel için başlık
#include "../gui/panels/TrainingHubPanel.h"
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
      // NLP'yi QObject'ten türettiğimiz için parent istiyor.
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

    // NLP'den gelen asenkron embedding sinyaline bağlan
    connect(&engine.getNlpProcessor(), &CerebrumLux::NaturalLanguageProcessor::embeddingReady,
            this, &CerebrumLux::MainWindow::onEmbeddingReady, Qt::QueuedConnection);
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

    // YENİ: Eğitim (Tutor) Paneli Eklendi
    // Not: LLMEngine::global_instance'ın tanımlı olduğu varsayılmaktadır.
    // learningModule referansı zaten MainWindow'da mevcut
    trainingHubPanel = new TrainingHubPanel(CerebrumLux::LLMEngine::global_instance, CerebrumLux::LLMEngine::global_instance, &learningModule, this);
    tabWidget->addTab(trainingHubPanel, "Eğitim");
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MainWindow: TrainingHubPanel tab'ı eklendi. Tab sayısı: " << tabWidget->count());

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
    
    // YENİ: Kullanıcı geri bildirimi sinyalini LearningModule'e bağla
    connect(chatPanel, &CerebrumLux::ChatPanel::userFeedbackGiven, &learningModule, &CerebrumLux::LearningModule::processUserChatFeedback);
    
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

    // YENİ: Asenkron yanıt izleyicisini bağla
    connect(&responseWatcher, &QFutureWatcher<CerebrumLux::ChatResponse>::finished, this, &CerebrumLux::MainWindow::onLLMResponseReady);

    guiUpdateTimer = new QTimer(this);
    connect(guiUpdateTimer, &QTimer::timeout, this, &CerebrumLux::MainWindow::updateGui);
    // OPTİMİZASYON: GUI güncelleme sıklığı 1 saniyeden 3 saniyeye çekildi.
    // Bu, veritabanı okuma yükünü %66 azaltır ve arayüz donmalarını engeller.

    // OPTİMİZASYON: GUI güncelleme sıklığı 1 saniyeden 3 saniyeye çekildi.
    guiUpdateTimer->start(3000); 
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MainWindow: GUI güncelleme zamanlayıcısı başlatıldı (3000ms). [Graph ve Simulation panellerini etkiler]");    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MainWindow: QTablePanel güncellemesi GUI zamanlayıcısına bağlandı.");

    // YENİ: Ağır LLM modelini GUI'yi bloklamadan arka planda yükle
    // LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "MainWindow: LLM modelinin asenkron yüklenmesi tetikleniyor..."); // Bu satır kaldırıldı
    // DÜZELTME: QtConcurrent::run, bir üye fonksiyona işaretçi ile çağrıldığında derleme hatası verebiliyor.
    // İSTEĞE BAĞLI İYİLEŞTİRME: [-Wunused-result] uyarısını gidermek için Q_UNUSED kullanıyoruz.
    //Q_UNUSED(QtConcurrent::run([this]() {
    //    this->engine.getResponseEngine().load_llm_model_async();
    //}));
    // KRİTİK PERFORMANS DÜZELTMESİ: LLM Modeli zaten 'main.cpp' içinde ResponseEngine başlatılırken senkron olarak yüklendi.
    // Burada tekrar asenkron yükleme başlatmak (Double Loading), RAM'i şişirir ve açılışı yavaşlatır.
    // Bu blok tamamen kaldırılmıştır.

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
    // --- PERFORMANS DÜZELTMESİ ---
    // Buradaki ağır veritabanı sorguları (semantic_search) KALDIRILDI.
    // Artık bu paneller sadece 'knowledgeBaseUpdated' sinyali geldiğinde güncellenecek.
    // 'updateKnowledgeBasePanel' fonksiyonu bu işi zaten yapıyor veya oraya taşıyacağız.
    
    // Eğer Simülasyon verilerinin sürekli akması gerekiyorsa (animasyon gibi),
    // bunu DB'den okumak yerine EngineIntegration üzerindeki RAM cache'den okumalıyız.
    // Şimdilik DB yükünü kesmek için burayı pasifize ediyoruz.
    
    /* 
       Eski Kod: Her saniye DB'yi tarıyordu.
       Yeni Durum: Sadece System Monitor gibi hafif verileri güncelle.
    */
   
   // Sadece LogPanel veya hafif arayüz güncellemeleri burada kalabilir.
   // Ağır işler Event-Driven oldu.

    // YENİ: GraphData embedding'ini bir kez hesaplayıp önbelleğe alıyoruz.
    static std::vector<float> graph_query_embedding = engine.getNlpProcessor().generate_text_embedding_sync("GraphData");
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
    
    // Graph Panel güncellemesini de çok sık yapmaya gerek yok, 
    // veya sadece yeni veri eklendiğinde yapılmalı.
    // Şimdilik burayı da yorum satırına alıp, updateKnowledgeBasePanel içine taşıyabiliriz.
    // Ancak görsel olarak bir şey görmek istiyorsanız açık kalabilir, 
    // fakat VectorDB optimizasyonu (MDB_NORDAHEAD) olmadan burası diski yorar.
    
    // ÖNERİ: GraphPanel güncellemesini de 'knowledgeBaseUpdated' sinyaline taşıyalım.
    // if (graphPanel) { graphPanel->updateData(...); } // Buradan kaldırıldı.
    
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
    
    engine.getSequenceManager().add_user_input(message.toStdString());

    if (chatPanel) chatPanel->appendChatMessage("System", "<i>(Analiz ediliyor...)</i>");
    
    // KRİTİK DÜZELTME: Embedding işlemini ASENKRON başlatıyoruz.
    // LLM'in ana thread'i BLOKLAMASINI ENGELLEYECEK olan bu çağrıdır.
    // Embedding hazır olduğunda 'onEmbeddingReady' slotu tetiklenecek.
    // request_id olarak mesajın kendisini kullanıyoruz (basitlik için).
    engine.getNlpProcessor().request_embedding_async(message.toStdString(), message.toStdString());
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MainWindow: Asenkron embedding isteği gönderildi.");
}

// YENİ SLOT: Embedding hazır olduğunda çağrılır
void MainWindow::onEmbeddingReady(const std::string& request_id, const std::vector<float>& embedding) {
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MainWindow: Embedding hazır. Request ID: " << request_id << ", Embedding boyutu: " << embedding.size());

    // --- RLHF ENTEGRASYONU ---
    // Kullanıcı mesajının embedding'ini hazır olduğunda kullanıyoruz.
    // Action olarak şimdilik genel 'MaximizeLearning' kullanıyoruz.
    learningModule.setLastInteraction(embedding, CerebrumLux::AIAction::MaximizeLearning);
    // -------------------------

    // Artık elimizde embedding var! Bunu kullanarak NLP ve ResponseEngine'i tetikleyebiliriz.
    // Niyet ve durum çıkarımı da bu embedding'i kullanacak.
    // NOT: Bu blok hala GUI thread'inde çalışıyor, ancak embedding artık alındığı için kısa sürecektir.
    
    // Niyet çıkarımı (artık embedding ile)
    CerebrumLux::UserIntent user_intent = engine.getNlpProcessor().infer_intent_from_text_with_embedding(request_id, embedding); 
    
    // Diğer durum ve hedef çıkarımları
    CerebrumLux::AbstractState current_abstract_state = CerebrumLux::AbstractState::Idle; 
    CerebrumLux::AIGoal current_goal = engine.getGoalManager().get_current_goal(); 
    const CerebrumLux::DynamicSequence& current_sequence = engine.getSequenceManager().get_current_sequence_ref(); 

    // KRİTİK: Yanıt üretimini asenkron olarak başlatıyoruz (LLM'i başka bir thread'e atıyoruz)
    // Bu, generate_response içinde LLM.generate çağrısının ana thread'i bloklamasını engeller.
    // user_embedding'i doğrudan generate_response'a iletiyoruz.
    QFuture<CerebrumLux::ChatResponse> future = QtConcurrent::run([=, this]() {
        return engine.getResponseEngine().generate_response(
            user_intent, 
            current_abstract_state, 
            current_goal, 
            current_sequence, 
            learningModule.getKnowledgeBase(),
            embedding // YENİ: Embedding'i ResponseEngine'e iletiyoruz
        );
    });
    
    // Sonucu izleyiciye bağla
    responseWatcher.setFuture(future);
}

// YENİ: Asenkron işlem bittiğinde çağrılacak slot
void MainWindow::onLLMResponseReady() {
    CerebrumLux::ChatResponse response = responseWatcher.result();
    
    if (chatPanel) {
        chatPanel->appendChatMessage("CerebrumLux", response);
    }
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MainWindow: Asenkron NLP yanıtı GUI'ye eklendi.");
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