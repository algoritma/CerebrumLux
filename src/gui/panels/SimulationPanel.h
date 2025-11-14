#ifndef SIMULATION_PANEL_H
#define SIMULATION_PANEL_H

#include <QWidget>
#include <QLineEdit>        // QLineEdit için
#include <QPushButton>      // QPushButton için
#include <QTableView>       // QTableView için
#include <QStandardItemModel> // QStandardItemModel için
#include <QVector>          // QVector için
#include <QLabel>           // YENİ EKLENDİ: QLabel sınıfı için gerekli başlık
#include <QTextEdit>        // YENİ: Chat geçmişi için

#include "../../core/logger.h" // LOG_DEFAULT makrosu için
#include "../DataTypes.h" // CerebrumLux::SimulationData için

#include <QVector>
#include <QDateTime> // SimulationData için
#include "../../data_models/dynamic_sequence.h" // DynamicSequence için
#include "../../communication/natural_language_processor.h" // CerebrumLux::ChatResponse için
#include "../../core/enums.h" // LogLevel için


namespace CerebrumLux {

class SimulationPanel : public QWidget
{
    Q_OBJECT
public:
    explicit SimulationPanel(QWidget *parent = nullptr);

public slots:
    void updateSimulationHistory(const QVector<CerebrumLux::SimulationData>& data);

signals:
    void commandEntered(const QString& command);
    void startSimulationTriggered();
    void stopSimulationTriggered();

private slots:
    void onCommandLineEditReturnPressed();
    void onStartSimulationClicked();
    void onStopSimulationClicked();

private:
    QLineEdit *commandLineEdit;
    QPushButton *sendCommandButton;
    QPushButton *startSimulationButton;
    QPushButton *stopSimulationButton;
    QLabel      *simulationStatusLabel; // YENİ: Simülasyon durumunu göstermek için
    QTableView *simulationHistoryTable;
    QStandardItemModel *simulationHistoryModel;

};

} // namespace CerebrumLux

#endif // SIMULATION_PANEL_H