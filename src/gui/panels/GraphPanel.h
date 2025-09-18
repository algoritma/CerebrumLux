#ifndef GRAPHPANEL_H
#define GRAPHPANEL_H

#include <QWidget>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include "../engine_integration.h" // LogData ve GraphData için
#include "DataTypes.h" 

class GraphPanel : public QWidget
{
    Q_OBJECT
public:
    explicit GraphPanel(QWidget *parent = nullptr);
    void addDataPoint(double x, double y);
    void updateGraph(size_t value); // YENİ: Grafik güncelleme metodu

private:
    QChart* chart;
    QChartView* chartView;
    QLineSeries* series;
};

#endif // GRAPHPANEL_H