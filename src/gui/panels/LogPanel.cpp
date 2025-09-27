#include "LogPanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollBar>
#include <QRegularExpressionMatch>
#include "../core/logger.h"
#include <QDateTime>

namespace CerebrumLux {

LogPanel::LogPanel(QWidget *parent) : QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    logTextEdit = new QTextEdit(this);
    logTextEdit->setReadOnly(true);
    mainLayout->addWidget(logTextEdit);

    QHBoxLayout *controlLayout = new QHBoxLayout();

    searchLineEdit = new QLineEdit(this);
    searchLineEdit->setPlaceholderText("Loglarda ara...");
    controlLayout->addWidget(searchLineEdit);

    clearLogButton = new QPushButton("Logu Temizle", this);
    controlLayout->addWidget(clearLogButton);

    mainLayout->addLayout(controlLayout);

    connect(clearLogButton, &QPushButton::clicked, this, &CerebrumLux::LogPanel::onClearLogClicked);
    connect(searchLineEdit, &QLineEdit::textChanged, this, &CerebrumLux::LogPanel::onSearchTextChanged);
    
    connect(logTextEdit->verticalScrollBar(), &QScrollBar::rangeChanged,
            [this](int min, int max){ 
                if (searchLineEdit->text().isEmpty()) { 
                    Q_UNUSED(min); logTextEdit->verticalScrollBar()->setValue(max); 
                }
            });

    LOG_DEFAULT(LogLevel::INFO, "LogPanel: Initialized.");
}

QString LogPanel::formatLogMessage(CerebrumLux::LogLevel level, const QString& message) const {
    QString formattedMessage;
    switch (level) {
        case LogLevel::TRACE:       formattedMessage = "<font color=\"gray\">" + message + "</font>"; break;
        case LogLevel::DEBUG:       formattedMessage = "<font color=\"blue\">" + message + "</font>"; break;
        case LogLevel::INFO:        formattedMessage = "<font color=\"white\">" + message + "</font>"; break;
        case LogLevel::WARNING:     formattedMessage = "<font color=\"orange\">" + message + "</font>"; break;
        case LogLevel::ERR_CRITICAL: formattedMessage = "<font color=\"red\">" + message + "</font>"; break;
        default:                    formattedMessage = message; break;
    }
    return formattedMessage;
}


void LogPanel::appendLog(CerebrumLux::LogLevel level, const QString& message) {
    // Logu dahili vektöre kaydet
    QString formattedMsg = formatLogMessage(level, message); // Formatlanmış mesajı burada oluştur
    originalLogs.push_back({level, message, message, formattedMsg}); // YENİ: formattedMessage alanını doldur

    // Eğer arama kutusu boşsa veya yeni log arama metnine uyuyorsa QTextEdit'e ekle
    if (searchLineEdit->text().isEmpty() || message.contains(searchLineEdit->text(), Qt::CaseInsensitive)) {
        logTextEdit->append(formattedMsg); // Formatlanmış mesajı kullan
    }
}

QTextEdit* LogPanel::getLogTextEdit() const {
    return logTextEdit;
}

void LogPanel::onClearLogClicked() {
    logTextEdit->clear();
    originalLogs.clear();
    LOG_DEFAULT(LogLevel::INFO, "LogPanel: Log içeriği ve dahili loglar temizlendi.");
    emit logCleared();
}

void LogPanel::onSearchTextChanged(const QString& text) {
    filterLogs(text);
}

void LogPanel::filterLogs(const QString& filterText) {
    logTextEdit->clear();

    if (filterText.isEmpty()) {
        for (const auto& logEntry : originalLogs) {
            logTextEdit->append(logEntry.formattedMessage);
        }
    } else {
        for (const auto& logEntry : originalLogs) {
            if (logEntry.rawMessage.contains(filterText, Qt::CaseInsensitive)) {
                logTextEdit->append(logEntry.formattedMessage);
            }
        }
    }
}


} // namespace CerebrumLux