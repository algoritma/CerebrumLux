#ifndef GRAPH_PANEL_H
#define GRAPH_PANEL_H

#include <QWidget>
#include <QMap>
#include <QString>

// Çalışan eski kodunuzdaki gibi doğrudan QtCharts başlıklarını dahil ediyoruz.
// Bu başlıklar, QChart, QChartView, QLineSeries gibi türleri global scope'ta veya Qt'nin mekanizmasıyla görünür kılar.
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QValueAxis> // QtCharts/QValueAxis yerine doğrudan QValueAxis (Eski kodda bu şekildeydi)
// QAbstractAxis'i GraphPanel.h'de dahil etmiyoruz çünkü eski kodda yoktu ve QValueAxis yeterli olabilir.

// QPainter için gerekebilir (QChartView render hint için)
#include <QPainter>

namespace CerebrumLux {

class GraphPanel : public QWidget
{
    Q_OBJECT
public:
    explicit GraphPanel(QWidget *parent = nullptr);
    void updateData(const QString& seriesName, const QMap<qreal, qreal>& data); // Güncelleme için mevcut metod adını koruyorum
    // Eski kodunuzdaki addDataPoint ve updateGraph(size_t) metotları bu sürümde yoktu,
    // ben updateData(const QString& seriesName, const QMap<qreal, qreal>& data); metodunu koruyorum.
    // Eğer bu metotları geri getirmek isterseniz, bana bildirin.

private:
    // Üyeleri eski çalışan kodunuzdaki gibi, QtCharts:: ön eki olmadan tanımlıyoruz.
    QChart *chart;
    QChartView *chartView;
    QLineSeries *series;
    QValueAxis *axisX; // Eski kodunuzdaki gibi QValueAxis
    QValueAxis *axisY; // Eski kodunuzdaki gibi QValueAxis
};


/*
using namespace QtCharts;

class GraphPanel : public QWidget
{
    Q_OBJECT
public:
    explicit GraphPanel(QWidget *parent = nullptr);
    void updateData(const QString& seriesName, const QMap<qreal, qreal>& data);

private:
    // Üyeleri niteliksiz isimleriyle tanımlıyoruz, çünkü 'using namespace QtCharts;' bu namespace içinde geçerli olacak.
    QChart *chart;
    QChartView *chartView;
    QLineSeries *series;
    QValueAxis *axisX;
    QValueAxis *axisY;
};
*/

} // namespace CerebrumLux

#endif // GRAPH_PANEL_H