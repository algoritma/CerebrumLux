#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QMap>
#include <QVector>
#include <QTimer> // QTimer için eklendi

// Tüm bağımlı sınıfların tam tanımları için başlık dosyalarını dahil ediyoruz
#include "../data_models/sequence_manager.h"
#include "../learning/LearningModule.h"
#include "../learning/KnowledgeBase.h"
#include "../gui/panels/LogPanel.h"
#include "../gui/panels/GraphPanel.h"
#include "../gui/panels/SimulationPanel.h"
#include "../gui/panels/CapsuleTransferPanel.h"
#include "../gui/engine_integration.h" // EngineIntegration için
#include "../gui/DataTypes.h" // CerebrumLux::SimulationData için (yeniden tanımlama olmaması için burada bırakıldı)
#include "ui_MainWindow.h" // Ui::MainWindow için (ÖNEMLİ: Bu dosya uic tarafından üretilir)


namespace CerebrumLux {

// SimulationData artık sadece DataTypes.h'den gelecek, burada yeniden tanımlamayacağız.
// struct SimulationData { ... }; // BU BLOK KALDIRILDI

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(EngineIntegration& engine, LearningModule& learningModule, QWidget *parent = nullptr);
    ~MainWindow();

    // Getter metotları için tam türleri kullanıyoruz
    LogPanel* getLogPanel() const;
    GraphPanel* getGraphPanel() const;
    SimulationPanel* getSimulationPanel() const;
    CapsuleTransferPanel* getCapsuleTransferPanel() const;

public slots:
    void appendLog(CerebrumLux::LogLevel level, const QString& message); // LogLevel namespace ile
    void updateGraphData(const QString& seriesName, const QMap<qreal, qreal>& data);
    void updateSimulationHistory(const QVector<CerebrumLux::SimulationData>& data); // SimulationData namespace ile
    void onSimulationCommandEntered(const QString& command);
    void onStartSimulationTriggered();
    void onStopSimulationTriggered();
    void onIngestCapsuleRequest(const QString& capsuleJson, const QString& signature, const QString& senderId);
    void onFetchWebCapsuleRequest(const QString& query);

private slots:
    void updateGui();

private:
    Ui::MainWindow *ui; // Ui::MainWindow tam tanımına artık sahibiz
    QTabWidget *tabWidget;
    LogPanel *logPanel; // Tam tanıma artık sahibiz
    GraphPanel *graphPanel; // Tam tanıma artık sahibiz
    SimulationPanel *simulationPanel; // Tam tanıma artık sahibiz
    CapsuleTransferPanel *capsuleTransferPanel; // Tam tanıma artık sahibiz

    EngineIntegration& engine;
    LearningModule& learningModule;

    QTimer *guiUpdateTimer;

    SimulationData convertCapsuleToSimulationData(const Capsule& capsule); // Namespace ile
};

} // namespace CerebrumLux

#endif // MAINWINDOW_H