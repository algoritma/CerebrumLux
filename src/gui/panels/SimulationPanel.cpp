#include "SimulationPanel.h"
#include <QVBoxLayout>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QStringList>

SimulationPanel::SimulationPanel(QWidget* parent) : QWidget(parent)
{
    layout = new QVBoxLayout(this);
    table = new QTableWidget(this);
    table->setColumnCount(2);
    table->setHorizontalHeaderLabels(QStringList() << "Step" << "Value");
    layout->addWidget(table);
    setLayout(layout);
}

void SimulationPanel::updatePanel(const std::vector<SimulationData>& data)
{
    table->setRowCount(static_cast<int>(data.size()));
    for (size_t i = 0; i < data.size(); ++i) {
        table->setItem(i, 0, new QTableWidgetItem(QString::number(data[i].step)));
        table->setItem(i, 1, new QTableWidgetItem(QString::number(data[i].value)));
    }
}