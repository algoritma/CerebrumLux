#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#pragma once
#include <QMainWindow>
#include <QTabWidget>
#include <QTimer>

class SimulationPanel; 
class LogPanel;      
class GraphPanel;    
class CapsuleTransferPanel; // YENİ: CapsuleTransferPanel için ileri bildirim

class EngineIntegration; 
class LearningModule;    

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(EngineIntegration& engine, LearningModule& learner, QWidget* parent = nullptr);
    ~MainWindow();

    LogPanel* getLogPanel() const { return logPanel; }
    CapsuleTransferPanel* getCapsuleTransferPanel() const { return capsuleTransferPanel; } // YENİ: Getter eklendi

private slots:
    void updateGui();
    void onSimulationCommandEntered(const QString& command);
    void onStartSimulationTriggered();
    void onStopSimulationTriggered();

    // YENİ: CapsuleTransferPanel'den gelen sinyaller için slotlar
    void onIngestCapsuleRequest(const QString& capsuleJson, const QString& signature, const QString& senderId);
    void onFetchWebCapsuleRequest(const QString& query);

private:
    EngineIntegration& engine;
    LearningModule& learningModule; 
                                    
    QTabWidget* tabWidget;
    SimulationPanel* simulationPanel;
    LogPanel* logPanel;
    GraphPanel* graphPanel;
    CapsuleTransferPanel* capsuleTransferPanel; // YENİ: Panel üyesi
    QTimer update_timer;
};

#endif // MAINWINDOW_H