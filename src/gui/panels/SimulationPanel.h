#ifndef SIMULATION_PANEL_H
#define SIMULATION_PANEL_H

#include <QWidget>
#include <QLineEdit>        // QLineEdit için
#include <QPushButton>      // QPushButton için
#include <QTableView>       // QTableView için
#include <QStandardItemModel> // QStandardItemModel için
#include <QVector>          // QVector için
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
    void appendChatMessage(const QString& sender, const CerebrumLux::ChatResponse& chatResponse); // DEĞİŞTİRİLDİ: Chat mesajlarını ve ChatResponse objesini ekler

signals:
    void commandEntered(const QString& command);
    void startSimulationTriggered();
    void stopSimulationTriggered();
    void chatMessageEntered(const QString& message); // YENİ: Chat: Kullanıcı chat mesajı girdiğinde (bu sinyal yine QString alır)

private slots:
    void onCommandLineEditReturnPressed();
    void onStartSimulationClicked();
    void onStopSimulationClicked();
    void onChatMessageLineEditReturnPressed(); // YENİ: Chat mesajı girişi için slot

private:
    QLineEdit *commandLineEdit;
    QPushButton *sendCommandButton;
    QPushButton *startSimulationButton;
    QPushButton *stopSimulationButton;
    QTableView *simulationHistoryTable;
    QStandardItemModel *simulationHistoryModel;

    // YENİ: Chat Arayüzü Bileşenleri
    QTextEdit *chatHistoryDisplay;
    QLineEdit *chatMessageLineEdit;
    QPushButton *sendChatMessageButton;
};

} // namespace CerebrumLux

#endif // SIMULATION_PANEL_H