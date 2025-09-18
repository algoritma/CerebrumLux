#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#pragma once
#include <QMainWindow>
#include <QTabWidget>
#include <QTimer>
// #include "EngineIntegration.h" // Tekrarlanan dahil etme kaldırıldı
#include "panels/SimulationPanel.h"
#include "panels/LogPanel.h"
#include "panels/GraphPanel.h"
// #include "learning/LearningModule.h" // Tekrarlanan dahil etme kaldırıldı

// İleri bildirimler
class EngineIntegration; // YENİ: EngineIntegration için ileri bildirim
class LearningModule;    // YENİ: LearningModule için ileri bildirim (eğer sadece referans tutuluyorsa)

// YENİ: Sadece bir kez dahil etme
#include "../src/gui/engine_integration.h" // Doğru yol ve tek dahil etme
#include "../src/learning/LearningModule.h" // Doğru yol ve tek dahil etme

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    // Kurucu imzası düzeltildi
    MainWindow(EngineIntegration& engine, LearningModule& learner, QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void updateGui();

private:
    EngineIntegration& engine;
    LearningModule& learningModule; // Pointer yerine referans tutulduğu varsayıldı
                                    // Eğer MainWindow LearningModule'ü sahipleniyorsa pointer kalabilir
                                    // Mevcut kodunuzda LearningModule* learningModule; idi,
                                    // ancak kurucuda LearningModule& learner alıyorsunuz.
                                    // Bu bir tutarsızlık. Referansla devam edelim.

    QTabWidget* tabWidget;
    SimulationPanel* simulationPanel;
    LogPanel* logPanel;
    GraphPanel* graphPanel;
    QTimer update_timer;
};

#endif // MAINWINDOW_H