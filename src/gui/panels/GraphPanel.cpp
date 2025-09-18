#include "GraphPanel.h"
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QVBoxLayout>
#include <QDebug> // YENİ: QDebug için eklendi
#include <QValueAxis> // YENİ: QValueAxis için eklendi (chart->axisX/Y()->setRange kullanmak için)


GraphPanel::GraphPanel(QWidget *parent) : QWidget(parent)
{
    series = new QLineSeries(this);
    chart = new QChart();
    chart->addSeries(series);
    chart->createDefaultAxes();
    chart->setTitle("Simulation Graph");

    // X ve Y eksenlerini doğru türe cast etmek için
    if (chart->axes(Qt::Horizontal).count() > 0) {
        auto axisX = qobject_cast<QValueAxis*>(chart->axes(Qt::Horizontal).at(0));
        if (axisX) {
            axisX->setTitleText("Zaman/Adım");
            axisX->setLabelFormat("%d");
        }
    }
    if (chart->axes(Qt::Vertical).count() > 0) {
        auto axisY = qobject_cast<QValueAxis*>(chart->axes(Qt::Vertical).at(0));
        if (axisY) {
            axisY->setTitleText("Değer");
            axisY->setLabelFormat("%.1f");
        }
    }


    chartView = new QChartView(chart, this);
    chartView->setRenderHint(QPainter::Antialiasing);

    auto layout = new QVBoxLayout(this);
    layout->addWidget(chartView);
    setLayout(layout);
}

void GraphPanel::addDataPoint(double x, double y)
{
    series->append(x, y);
}

// YENİ: Grafik güncelleme metodu implementasyonu
void GraphPanel::updateGraph(size_t value)
{
    // Mevcut grafiğe yeni bir veri noktası ekleyelim.
    // X ekseni için basitçe mevcut nokta sayısını kullanabiliriz.
    // Y ekseni için gelen 'value' parametresini kullanırız.
    double x_value = series->count(); // Mevcut nokta sayısı
    double y_value = static_cast<double>(value); // Gelen değeri Y ekseni değeri olarak kullan

    series->append(x_value, y_value);

    // Grafik aralığını otomatik olarak güncelle
    if (chart->axes(Qt::Horizontal).count() > 0) {
        auto axisX = qobject_cast<QValueAxis*>(chart->axes(Qt::Horizontal).at(0));
        if (axisX) {
            axisX->setRange(0, series->count());
        }
    }
    
    // Y ekseni için min/max değerlerini otomatik ayarla veya sabit tut
    if (series->count() > 0 && chart->axes(Qt::Vertical).count() > 0) {
        double minY = y_value, maxY = y_value;
        for (int i = 0; i < series->count(); ++i) {
            QPointF point = series->at(i);
            if (point.y() < minY) minY = point.y();
            if (point.y() > maxY) maxY = point.y();
        }
        auto axisY = qobject_cast<QValueAxis*>(chart->axes(Qt::Vertical).at(0));
        if (axisY) {
            // Sadece tek bir nokta varsa min/max aynı olacağından biraz boşluk bırak
            if (minY == maxY) { 
                axisY->setRange(minY - 1.0, maxY + 1.0);
            } else {
                axisY->setRange(minY - (maxY - minY) * 0.1, maxY + (maxY - minY) * 0.1); // %10 boşluk
            }
        }
    }

    // Qt debug çıktısı
    qDebug() << "[GraphPanel] Grafik güncellendi: (" << x_value << ", " << y_value << ")";
}