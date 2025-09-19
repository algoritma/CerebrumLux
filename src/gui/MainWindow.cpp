#include "MainWindow.h"
#include <QTimer>
#include <iostream>
#include <QStringList> 
#include <QDebug> 
#include "../core/logger.h" // YENİ: Logger için dahil edildi


// YENİ: Capsule'dan SimulationData'ya basit bir dönüştürücü (gerekirse DataTypes.h'ye taşınabilir)
// Bu fonksiyonun, Capsule ve SimulationData arasındaki veri modelini bildiği varsayılır.
SimulationData convertCapsuleToSimulationData(const Capsule& capsule) {
    SimulationData data;
    // Örnek dönüşüm: Capsule'ın içeriğini veya ID'sini kullan
    // Gerçek bir senaryoda, Capsule'ın ilgili alanları SimulationData'nın alanlarına eşlenir.
    data.id = capsule.id; 
    data.value = capsule.confidence; 
    // data.label = QString::fromStdString(capsule.topic); // Veya başka bir alan
    return data;
}


// Constructor
MainWindow::MainWindow(EngineIntegration& eng, LearningModule& learn, QWidget* parent)
    : QMainWindow(parent), engine(eng), learningModule(learn) // learningModule referans olarak başlatıldı
{
    // LearningModule artık dışarıdan referans olarak geliyor, bu yüzden burada new kullanmıyoruz.
    // engine.setLearningModule(learningModule); // EngineIntegration'da setLearningModule kaldırıldı/değiştirildi

    // Paneller
    tabWidget = new QTabWidget(this);
    simulationPanel = new SimulationPanel(this);
    logPanel = new LogPanel(this);
    graphPanel = new GraphPanel(this);

    tabWidget->addTab(simulationPanel, "Simulation");
    tabWidget->addTab(logPanel, "Log");
    tabWidget->addTab(graphPanel, "Graph");

    setCentralWidget(tabWidget);

    // YENİ: LogPanel'i Logger'a kaydet
    Logger::get_instance().set_log_panel(logPanel); // DÜZELTİLDİ

    // GUI güncelleme timer
    connect(&update_timer, &QTimer::timeout, this, &MainWindow::updateGui);
    update_timer.start(500); // 500 ms'de bir güncelleme
    // Not: İki farklı QTimer tanımlanmış, bir tanesi gereksiz olabilir. İkincisi kaldırıldı.
}

// Yıkıcı
MainWindow::~MainWindow() {
    // learningModule artık referans olduğu için delete kullanılmaz.
    // Panelleri silme (eğer parent'ı this ise Qt otomatik yönetir)
    // tabWidget, simulationPanel, logPanel, graphPanel delete edilmiyor çünkü QObject hiyerarşisi onları yönetiyor.
}

// GUI güncelleme metodu
void MainWindow::updateGui() {
    // Simulation panel güncellemesi
    // getCapsulesByTopic'ten dönen Capsule'ları SimulationData'ya dönüştür
    std::vector<Capsule> capsules_for_sim = engine.getKnowledgeBase().getCapsulesByTopic("StepSimulation");
    std::vector<SimulationData> simulation_data_vec;
    for (const auto& cap : capsules_for_sim) {
        simulation_data_vec.push_back(convertCapsuleToSimulationData(cap)); // Dönüşüm fonksiyonu kullanıldı
    }
    simulationPanel->updatePanel(simulation_data_vec); // Düzeltildi

    // Log panel güncellemesi: Artık Logger tarafından doğrudan LogPanel'e yazılıyor
    // Buradaki manuel güncelleme kaldırıldı.
    // QStringList q_logs; 
    // q_logs.append(QString::fromStdString(engine.getLatestLogs())); 
    // logPanel->updatePanel(q_logs); 


    // Graph panel güncelleme
    auto capsules_for_graph = learningModule.getCapsulesByTopic("StepSimulation"); // Düzeltildi
    graphPanel->updateGraph(capsules_for_graph.size()); 

    std::cout << "[GUI] Güncelleme..." << std::endl; 
    qDebug() << "[Qt GUI] Güncelleme..."; 

    // Öğrenilen bilgileri test amaçlı alalım
    auto results = learningModule.getKnowledgeBase().findSimilar("Qt6", 2); // Düzeltildi
    if (!results.empty()) {
        std::cout << "[GUI] Hatırlanan bilgi: " << results[0].content << std::endl;
        qDebug() << "[Qt GUI] Hatırlanan bilgi: " << QString::fromStdString(results[0].content);
    }
}