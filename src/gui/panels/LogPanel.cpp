#include "LogPanel.h"
#include <QVBoxLayout>
#include <QTextBlock> // QTextEdit'teki blokları saymak için
#include "../../core/logger.h" // LOG_DEFAULT makrosu için

namespace CerebrumLux { // TÜM İMPLEMENTASYON BU NAMESPACE İÇİNDE OLACAK

CerebrumLux::LogPanel::LogPanel(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    this->logTextEdit = new QTextEdit(this);
    this->logTextEdit->setReadOnly(true);
    this->logTextEdit->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    this->logTextEdit->setFontFamily("Consolas"); // Loglar için mono font
    this->logTextEdit->setFontPointSize(9);
    mainLayout->addWidget(this->logTextEdit);

    this->clearLogButton = new QPushButton("Logu Temizle", this);
    mainLayout->addWidget(this->clearLogButton);

    connect(this->clearLogButton, &QPushButton::clicked, this, &CerebrumLux::LogPanel::onClearLogClicked);

    setLayout(mainLayout);

    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "LogPanel: Initialized.");
}

void CerebrumLux::LogPanel::appendLog(CerebrumLux::LogLevel level, const QString& message) {
    // Log seviyesine göre renk kodlaması yap
    QString coloredMessage = message;
    if (level == CerebrumLux::LogLevel::ERR_CRITICAL) {
        coloredMessage = "<font color='red'>" + message + "</font>";
    } else if (level == CerebrumLux::LogLevel::WARNING) {
        coloredMessage = "<font color='orange'>" + message + "</font>";
    } else if (level == CerebrumLux::LogLevel::INFO) {
        coloredMessage = "<font color='white'>" + message + "</font>";
    } else if (level == CerebrumLux::LogLevel::DEBUG) {
        coloredMessage = "<font color='gray'>" + message + "</font>";
    } else if (level == CerebrumLux::LogLevel::TRACE) {
        coloredMessage = "<font color='lightgray'>" + message + "</font>";
    }
    this->logTextEdit->append(coloredMessage);
    // Maksimum satır sayısını kontrol et (bellek tüketimini azaltmak için)
    const int MAX_LINES = 1000;
    if (this->logTextEdit->document()->blockCount() > MAX_LINES) {
        QTextCursor cursor(this->logTextEdit->document());
        cursor.movePosition(QTextCursor::Start);
        for (int i = 0; i < this->logTextEdit->document()->blockCount() - MAX_LINES; ++i) {
            cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
        }
        cursor.removeSelectedText();
        cursor.deleteChar(); // Yeni satırı sil
    }
}

QTextEdit* CerebrumLux::LogPanel::getLogTextEdit() const {
    return this->logTextEdit;
}

void CerebrumLux::LogPanel::onClearLogClicked() {
    this->logTextEdit->clear();
    emit logCleared(); // Log temizlendi sinyalini yay
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "LogPanel: Log cleared via GUI button.");
}

} // namespace CerebrumLux