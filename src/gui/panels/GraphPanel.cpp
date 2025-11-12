#include "GraphPanel.h"

// QtCharts başlıkları GraphPanel.h içinde zaten dahil edildiği için burada tekrar etmiyoruz.
// QPainter zaten GraphPanel.h'de dahil edildiği için burada tekrar etmiyoruz.

#include <QVBoxLayout>
#include "../../core/logger.h" // LOG_DEFAULT makrosu için
#include <limits> // std::numeric_limits için
#include <QDebug> // Eski kodunuzdaki gibi QDebug için eklendi

// using namespace QtCharts; // Eski çalışan kodunuzda bu yoktu, bu yüzden kaldırıldı.

namespace CerebrumLux { // TÜM İMPLEMENTASYON BU NAMESPACE İÇİNDE OLACAK

CerebrumLux::GraphPanel::GraphPanel(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Eski çalışan kodunuzdaki gibi, QtCharts:: ön eki olmadan kullanıyoruz.
    this->chart = new QChart();
    this->chart->setTitle("AI Güven Seviyesi ve Performans Grafiği");
    this->chart->legend()->hide();

    // X ve Y eksenlerini oluştur
    this->axisX = new QValueAxis(); // Eski çalışan kodunuzdaki gibi
    this->axisX->setTitleText("Zaman (ms)");
    this->axisX->setLabelFormat("%d");
    this->chart->addAxis(this->axisX, Qt::AlignBottom);

    this->axisY = new QValueAxis(); // Eski çalışan kodunuzdaki gibi
    this->axisY->setTitleText("Güven/Performans");
    this->axisY->setLabelFormat("%.2f");
    this->axisY->setRange(0, 1);
    this->chart->addAxis(this->axisY, Qt::AlignLeft);

    this->series = new QLineSeries();
    this->series->setName("AI Confidence");
    this->chart->addSeries(this->series);
    this->series->attachAxis(this->axisX); // Seriyi X eksenine bağla
    this->series->attachAxis(this->axisY); // Seriyi Y eksenine bağla

    this->chartView = new QChartView(this->chart);
    this->chartView->setRenderHint(QPainter::Antialiasing);
    mainLayout->addWidget(this->chartView);

    setLayout(mainLayout);

    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "GraphPanel: Initialized.");
}

void CerebrumLux::GraphPanel::updateData(const QString& seriesName, const QMap<qreal, qreal>& data) {
    if (seriesName == "AI Confidence" && this->series) {
        this->series->clear();
        qreal maxX = 0;
        qreal minX = std::numeric_limits<qreal>::max();
        qreal maxY = 0;
        qreal minY = std::numeric_limits<qreal>::max();

        for (auto it = data.constBegin(); it != data.constEnd(); ++it) {
            this->series->append(it.key(), it.value());
            maxX = std::max(maxX, it.key());
            minX = std::min(minX, it.key());
            maxY = std::max(maxY, it.value());
            minY = std::min(minY, it.value());
        }

        if (this->chart->axes(Qt::Horizontal).size() > 0 && this->chart->axes(Qt::Vertical).size() > 0) {
            // QValueAxis* türüne qobject_cast yaparken tam nitelikli ismi kullanıyoruz.
            QValueAxis *axisX_ptr = qobject_cast<QValueAxis*>(this->chart->axes(Qt::Horizontal).at(0));
            QValueAxis *axisY_ptr = qobject_cast<QValueAxis*>(this->chart->axes(Qt::Vertical).at(0));
            if (axisX_ptr && axisY_ptr) {
                // DÜZELTİLDİ: Eksen aralığı yönetimi. Veri varsa dinamik, yoksa varsayılan.
                if (data.isEmpty()) { 
                    axisX_ptr->setRange(0, 10000); // Varsayılan 10 saniye
                } else {
                    axisX_ptr->setRange(minX, maxX); // Tüm mevcut verinin aralığını göster
                }
                
                // YENİ: Dinamik Y ekseni aralığı. Confidence 0-1 arasında olmalı.
                // Eğer max_confidence 0 ise veya min_confidence çok yüksekse varsayılan 0-1 aralığını koru.
                axisY_ptr->setRange(std::max(0.0, minY - 0.05), std::min(1.0, maxY + 0.05)); // Biraz margin ekle
                if (minY == std::numeric_limits<qreal>::max() || maxY == std::numeric_limits<qreal>::lowest()) {
                    axisY_ptr->setRange(0, 1); // Veri yoksa veya geçersizse varsayılan
                }
            }
        }
    }
    LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "GraphPanel: Data updated for series: " << seriesName.toStdString());
}

} // namespace CerebrumLux