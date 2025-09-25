#ifndef SIMULATION_PANEL_H
#define SIMULATION_PANEL_H

#include <QWidget>
// İleri bildirimleri kaldırıp tam başlık dosyalarını dahil ediyoruz
#include <QLineEdit>        // QLineEdit için
#include <QPushButton>      // QPushButton için
#include <QTableView>       // QTableView için
#include <QStandardItemModel> // QStandardItemModel için
#include <QVector>          // QVector için

#include "../../core/logger.h" // LOG_DEFAULT makrosu için
#include "../DataTypes.h" // CerebrumLux::SimulationData için

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
    QTableView *simulationHistoryTable;
    QStandardItemModel *simulationHistoryModel;
};

} // namespace CerebrumLux

#endif // SIMULATION_PANEL_H