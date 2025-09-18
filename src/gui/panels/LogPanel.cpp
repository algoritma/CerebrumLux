#include "LogPanel.h"
#include <QVBoxLayout>

LogPanel::LogPanel(QWidget* parent) : QWidget(parent)
{
    logArea = new QTextEdit(this);
    logArea->setReadOnly(true);
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(logArea);
    setLayout(layout);
}

void LogPanel::updatePanel(const QStringList& logs)
{
    logArea->clear();
    for (const auto& log : logs) {
        logArea->append(log);
    }
}
