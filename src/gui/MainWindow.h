#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#pragma once
#include <QMainWindow>
#include <QTabWidget>
#include <QTimer>

class SimulationPanel; 
class LogPanel;      
class GraphPanel;    

class EngineIntegration; 
class LearningModule;    

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(EngineIntegration& engine, LearningModule& learner, QWidget* parent = nullptr);
    ~MainWindow();

    LogPanel* getLogPanel() const { return logPanel; }

private slots:
    void updateGui();
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