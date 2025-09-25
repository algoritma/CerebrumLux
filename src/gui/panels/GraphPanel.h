#ifndef GRAPH_PANEL_H
#define GRAPH_PANEL_H

#include <QWidget>
#include <QMap>
#include <QString>

// QtCharts sınıfları için tüm gerekli başlık dosyalarını buraya DAHİL EDİYORUZ.
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QAbstractAxis> // QValueAxis QAbstractAxis'ten türediği için gerekebilir

// QPainter için gerekebilir (QChartView render hint için)
#include <QPainter>

namespace CerebrumLux {

class GraphPanel : public QWidget
{
    Q_OBJECT
public:
    explicit GraphPanel(QWidget *parent = nullptr);
    void updateData(const QString& seriesName, const QMap<qreal, qreal>& data);

private:
    // Üyeleri doğrudan sınıf adıyla tanımlıyoruz (QtCharts:: ön eki olmadan)
    QChart *chart;
    QChartView *chartView;
    QLineSeries *series;
    QValueAxis *axisX;
    QValueAxis *axisY;
};

} // namespace CerebrumLux

#endif // GRAPH_PANEL_H