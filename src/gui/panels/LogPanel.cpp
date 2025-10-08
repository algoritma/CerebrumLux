#include "LogPanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollBar>
#include <QRegularExpressionMatch>
#include "../core/logger.h"
#include <QDateTime>
#include <QTextCharFormat>
#include <QBrush>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>

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

    // Logger singleton'ından gelen sinyali bu slot'a bağla
    // Qt::QueuedConnection, sinyal emit eden thread ile slot'un çalıştığı thread farklıysa güvenli iletişim sağlar.
    connect(&Logger::getInstance(), &Logger::messageLogged,
            this, &LogPanel::handleMessageLogged, Qt::QueuedConnection); 

    connect(clearLogButton, &QPushButton::clicked, this, &LogPanel::onClearLogClicked);
    connect(searchLineEdit, &QLineEdit::textChanged, this, &LogPanel::onSearchTextChanged);
    
    connect(logTextEdit->verticalScrollBar(), &QScrollBar::rangeChanged,
            [this](int min, int max){ 
                if (searchLineEdit->text().isEmpty()) { 
                    Q_UNUSED(min); 
                    logTextEdit->verticalScrollBar()->setValue(max); 
                }
            });

    // LogPanel'in başlangıçta boş olmaması için bir hoş geldiniz mesajı
    // Bu mesajı da kendi formatlayıcısından geçirerek ekleyelim.
    QString welcomeMessage = "Cerebrum Lux Log Paneli Başlatıldı.";
    QString formattedWelcome = formatLogMessage(LogLevel::INFO, welcomeMessage, "LogPanel.cpp", __LINE__);
    originalLogs.push_back({LogLevel::INFO, welcomeMessage, formattedWelcome, "LogPanel.cpp", __LINE__});
    logTextEdit->append(formattedWelcome);

    LOG_DEFAULT(LogLevel::INFO, "LogPanel: Modül Başlatıldı."); // Logger artık çalışıyor, bu log dosyaya/konsola ve GUI'ye gidecek
}

LogPanel::~LogPanel() {
    LOG_DEFAULT(LogLevel::INFO, "LogPanel: Modül Sonlandırıldı."); // Modül yıkılırken de log gönder
    // QObject parent-child mekanizması sayesinde bağlantılar otomatik olarak kopar.
}

void LogPanel::handleMessageLogged(CerebrumLux::LogLevel level, const QString& rawMessage, const QString& file, int line) {
    // Logger'dan gelen ham mesajı, LogPanel'in kendi formatlama mantığı ile renklendir ve tam mesajı oluştur.
    QString fullFormattedMessage = formatLogMessage(level, rawMessage, file, line);

    // Logu dahili vektöre kaydet (filtreleme için)
    originalLogs.push_back({level, rawMessage, fullFormattedMessage, file, line});

    // Eğer arama kutusu boşsa veya yeni log arama metnine uyuyorsa QTextEdit'e ekle
    if (searchLineEdit->text().isEmpty() || rawMessage.contains(searchLineEdit->text(), Qt::CaseInsensitive)) {
        logTextEdit->append(fullFormattedMessage);
    }
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
            logTextEdit->append(logEntry.formattedMessage); // Formatlanmış mesajı kullan
        }
    } else {
        QRegularExpression regex(filterText, QRegularExpression::CaseInsensitiveOption);
        for (const auto& logEntry : originalLogs) {
            if (logEntry.rawMessage.contains(regex)) { // Ham mesaj üzerinde filtrele
                logTextEdit->append(logEntry.formattedMessage); // Formatlanmış mesajı göster
            }
        }
    }
}

QString LogPanel::formatLogMessage(CerebrumLux::LogLevel level, const QString& message, const QString& file, int line) const {
    QString formattedMessage;
    // Not: Logger'dan gelen mesaj zaten zaman damgası ve diğer bilgileri içeriyor.
    // Bu fonksiyon sadece renklendirme ekliyor.
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

} // namespace CerebrumLux