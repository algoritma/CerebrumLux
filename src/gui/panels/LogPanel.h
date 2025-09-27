#ifndef LOG_PANEL_H
#define LOG_PANEL_H

#include <QWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QDateTime>
#include <QLineEdit>
#include <QRegularExpression>
#include <QVBoxLayout>
#include <vector>

#include "../../core/enums.h"
#include "../../core/logger.h"


namespace CerebrumLux {

// YENİ: Log girişlerini saklamak için yapı
struct LogEntry {
    CerebrumLux::LogLevel level;
    QString message;
    QString rawMessage; // Filtreleme için ham mesajı sakla
    QString formattedMessage; // YENİ: Formatlanmış mesajı sakla
};

class LogPanel : public QWidget
{
    Q_OBJECT
public:
    explicit LogPanel(QWidget *parent = nullptr);
    void appendLog(CerebrumLux::LogLevel level, const QString& message);
    QTextEdit* getLogTextEdit() const;

signals:
    void logCleared();

private slots:
    void onClearLogClicked();
    void onSearchTextChanged(const QString& text);

private:
    QTextEdit *logTextEdit;
    QPushButton *clearLogButton;
    QLineEdit *searchLineEdit;

    std::vector<LogEntry> originalLogs;

    void filterLogs(const QString& filterText);
    QString formatLogMessage(CerebrumLux::LogLevel level, const QString& message) const;
};

} // namespace CerebrumLux

#endif // LOG_PANEL_H