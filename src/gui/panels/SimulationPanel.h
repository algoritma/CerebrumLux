#ifndef SIMULATIONPANEL_H
#define SIMULATIONPANEL_H

#include <QWidget>
#include <vector>
#include "../DataTypes.h" // SimulationData burada tanımlı

// Forward declarations for Qt classes
class QTableWidget;
class QVBoxLayout;

class SimulationPanel : public QWidget
{
    Q_OBJECT
public:
    SimulationPanel(QWidget* parent = nullptr);
    void updatePanel(const std::vector<SimulationData>& data);

private:
    // UI elemanları
    QTableWidget* table;
    QVBoxLayout* layout;
};

#endif // SIMULATIONPANEL_H
