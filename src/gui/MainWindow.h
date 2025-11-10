#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QMap>
#include <QVector>
#include <QTimer>

#include "../data_models/sequence_manager.h"
#include "../learning/LearningModule.h"
#include "../learning/KnowledgeBase.h"
#include "../gui/panels/LogPanel.h"
#include "../gui/panels/GraphPanel.h"
#include "../gui/panels/SimulationPanel.h"
#include "../gui/panels/CapsuleTransferPanel.h"
#include "../gui/panels/KnowledgeBasePanel.h"
#include "../gui/panels/QTablePanel.h" // YENİ: QTablePanel için başlık
#include "../communication/natural_language_processor.h" // CerebrumLux::ChatResponse için
#include "../gui/engine_integration.h"
#include "../gui/DataTypes.h"
#include "ui_MainWindow.h"


namespace CerebrumLux {

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(EngineIntegration& engine, LearningModule& learningModule, QWidget *parent = nullptr);
    ~MainWindow();

    LogPanel* getLogPanel() const;
    GraphPanel* getGraphPanel() const;
    SimulationPanel* getSimulationPanel() const;
    CapsuleTransferPanel* getCapsuleTransferPanel() const;
    KnowledgeBasePanel* getKnowledgeBasePanel() const;
    QTablePanel* getQTablePanel() const; // YENİ: QTablePanel için getter

 
    void updateGraphData(const QString& seriesName, const QMap<qreal, qreal>& data);
    void updateSimulationHistory(const QVector<CerebrumLux::SimulationData>& data);
    void onSimulationCommandEntered(const QString& command);
    void onStartSimulationTriggered();
    void onStopSimulationTriggered();
    void onIngestCapsuleRequest(const QString& capsuleJson, const QString& signature, const QString& senderId);
    void onFetchWebCapsuleRequest(const QString& query);

private slots:
    void updateGui();
    void updateKnowledgeBasePanel();
    void onWebFetchCompleted(const CerebrumLux::IngestReport& report);
    void onChatMessageReceived(const QString& message); // YENİ: Chat mesajı işleme slotu

private:
    Ui::MainWindow *ui;
    QTabWidget *tabWidget;
    LogPanel *logPanel;
    GraphPanel *graphPanel;
    SimulationPanel *simulationPanel;
    CapsuleTransferPanel *capsuleTransferPanel;
    KnowledgeBasePanel *knowledgeBasePanel;
    QTablePanel *qTablePanel; // YENİ: QTablePanel üyesi

    EngineIntegration& engine;
    LearningModule& learningModule;

    QTimer *guiUpdateTimer;

    SimulationData convertCapsuleToSimulationData(const Capsule& capsule);
};

} // namespace CerebrumLux

#endif // MAINWINDOW_H