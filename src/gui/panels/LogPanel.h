#ifndef LOG_PANEL_H
#define LOG_PANEL_H

#include <QWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QDateTime> // QDateTime için

#include "../../core/enums.h" // CerebrumLux::LogLevel için

namespace CerebrumLux { // LogPanel sınıfı bu namespace içine alınacak

class LogPanel : public QWidget
{
    Q_OBJECT
public:
    explicit LogPanel(QWidget *parent = nullptr);
    void appendLog(CerebrumLux::LogLevel level, const QString& message);
    QTextEdit* getLogTextEdit() const; // LogTextEdit'e erişim için getter

signals:
    void logCleared(); // Log temizleme sinyali

private slots:
    void onClearLogClicked();

private:
    QTextEdit *logTextEdit;
    QPushButton *clearLogButton;
};

} // namespace CerebrumLux

#endif // LOG_PANEL_H