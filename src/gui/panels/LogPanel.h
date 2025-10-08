#ifndef LOG_PANEL_H
#define LOG_PANEL_H

#include <QWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QDateTime>
#include <QVBoxLayout>
#include <vector>
#include <QLineEdit>
#include <QRegularExpression>
#include <QTextCharFormat>

#include "../../core/enums.h"

namespace CerebrumLux {

struct LogEntry {
    CerebrumLux::LogLevel level;
    QString rawMessage;
    QString formattedMessage;
    QString file;
    int line;
};

class LogPanel : public QWidget
{
    Q_OBJECT
public:
    explicit LogPanel(QWidget *parent = nullptr);
    ~LogPanel() override;

signals:
    void logCleared();

private slots:
    void handleMessageLogged(CerebrumLux::LogLevel level, const QString& rawMessage, const QString& file, int line);
    void onClearLogClicked();
    void onSearchTextChanged(const QString& text);

private:
    QTextEdit *logTextEdit;
    QPushButton *clearLogButton;
    QLineEdit *searchLineEdit;

    std::vector<LogEntry> originalLogs;
    
    void filterLogs(const QString& filterText);
    QString formatLogMessage(CerebrumLux::LogLevel level, const QString& message, const QString& file, int line) const;
};

} // namespace CerebrumLux

#endif // LOG_PANEL_H