#include "LogPanel.h"
#include <QVBoxLayout>
#include <QDebug> // QDebug kullanılıyorsa dahil edilmeli
#include <QScrollBar> // YENİ: QScrollBar için eklendi

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
    // DÜZELTİLDİ: logArea->clear() çağrısı kaldırıldı.
    // Artık her yeni log mesajı mevcut içeriğin sonuna eklenecek.
    for (const auto& log : logs) {
        logArea->append(log);
    }
    // İsteğe bağlı: En alta kaydırma
    logArea->verticalScrollBar()->setValue(logArea->verticalScrollBar()->maximum());
}