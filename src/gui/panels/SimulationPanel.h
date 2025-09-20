#ifndef SIMULATIONPANEL_H
#define SIMULATIONPANEL_H

#include <QWidget>
#include <vector>
#include "../DataTypes.h" // SimulationData burada tanımlı

// Forward declarations for Qt classes
class QTableWidget;
class QVBoxLayout;
class QHBoxLayout;  // YENİ: Düğmeler için
class QLineEdit;    // YENİ: Metin girişi için
class QPushButton;  // YENİ: Düğmeler için

class SimulationPanel : public QWidget
{
    Q_OBJECT
public:
    explicit SimulationPanel(QWidget* parent = nullptr);
    void updatePanel(const std::vector<SimulationData>& data);

signals:
    // YENİ: Kullanıcıdan alınan komutu dışarıya iletmek için sinyal
    void commandEntered(const QString& command);
    // YENİ: Simülasyonu başlatma sinyali
    void startSimulation();
    // YENİ: Simülasyonu durdurma sinyali
    void stopSimulation();

private slots:
    // YENİ: Kullanıcı metin girdisi tamamladığında çağrılacak slot
    void onCommandLineEditReturnPressed();
    // YENİ: Simülasyon başlat düğmesine basıldığında çağrılacak slot
    void onStartSimulationClicked();
    // YENİ: Simülasyon durdur düğmesine basıldığında çağrılacak slot
    void onStopSimulationClicked();

private:
    // UI elemanları
    QTableWidget* table;
    QVBoxLayout* layout;

    // YENİ UI elemanları
    QLineEdit* commandLineEdit;    // Kullanıcıdan komut almak için
    QPushButton* startButton;      // Simülasyonu başlatma düğmesi
    QPushButton* stopButton;       // Simülasyonu durdurma düğmesi
    QHBoxLayout* controlLayout;    // Düğmeleri ve giriş alanını düzenlemek için
};

#endif // SIMULATIONPANEL_H