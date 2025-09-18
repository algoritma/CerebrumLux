#ifndef LOGPANEL_H
#define LOGPANEL_H

#include <QWidget>
#include <QTextEdit>
#include <QStringList>

#include "../engine_integration.h" // LogData ve GraphData için

class LogPanel : public QWidget
{
    Q_OBJECT
public:
    explicit LogPanel(QWidget* parent = nullptr);
    void updatePanel(const QStringList& logs);

private:
    QTextEdit* logArea;
};

#endif // LOGPANEL_H
