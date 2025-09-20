#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#pragma once
#include <QMainWindow>
#include <QTabWidget>
#include <QTimer>

// İleri bildirimler
class SimulationPanel; 
class LogPanel;      
class GraphPanel;    

// Diğer ileri bildirimler ve include'lar
class EngineIntegration; 
class LearningModule;    

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(EngineIntegration& engine, LearningModule& learner, QWidget* parent = nullptr);
    ~MainWindow();

    // LogPanel'e erişim için getter metodu
    LogPanel* getLogPanel() const { return logPanel; }

private slots:
    void updateGui();
    // YENİ: SimulationPanel'den gelen sinyalleri işlemek için slot'lar
    void onSimulationCommandEntered(const QString& command);
    void onStartSimulationTriggered();
    void onStopSimulationTriggered();

private:
    EngineIntegration& engine;
    LearningModule& learningModule; 
                                    
    QTabWidget* tabWidget;
    SimulationPanel* simulationPanel;
    LogPanel* logPanel;
    GraphPanel* graphPanel;
    QTimer update_timer;
};

#endif // MAINWINDOW_H