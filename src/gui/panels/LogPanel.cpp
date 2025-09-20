#include "LogPanel.h"
#include <QVBoxLayout> 
#include <QLineEdit>   
#include <QTextCharFormat> 
#include <QTextCursor>     
#include <QScrollBar> // YENİ: Kaydırma çubuğuna erişim için

LogPanel::LogPanel(QWidget* parent) : QWidget(parent)
{
    mainLayout = new QVBoxLayout(this); 

    searchLineEdit = new QLineEdit(this); 
    searchLineEdit->setPlaceholderText("Search logs...");
    mainLayout->addWidget(searchLineEdit); 

    logArea = new QTextEdit(this);
    logArea->setReadOnly(true); 
    mainLayout->addWidget(logArea);

    setLayout(mainLayout); 

    connect(searchLineEdit, &QLineEdit::textChanged, this, &LogPanel::onSearchTextChanged);
}

void LogPanel::updatePanel(const QStringList& logs)
{
    // Gelen logları dahili buffer'a ekle
    allLogsBuffer.append(logs);

    // Arama metni boş değilse, filtreleme yap
    if (!searchLineEdit->text().isEmpty()) {
        onSearchTextChanged(searchLineEdit->text()); // Mevcut arama metnine göre yeniden filtrele
    } else {
        // Arama metni boşsa, tüm yeni logları doğrudan append et
        // allLogsBuffer'ı temizleyip yeniden inşa etmek yerine, sadece yeni gelenleri ekleyelim
        // Bu daha performanslıdır.
        for (const QString& log : logs) {
            logArea->append(log);
        }
        // En alta kaydır
        logArea->verticalScrollBar()->setValue(logArea->verticalScrollBar()->maximum());
    }
}

// Arama kutusundaki metin değiştiğinde tetiklenen slot
void LogPanel::onSearchTextChanged(const QString& searchText)
{
    logArea->clear(); // QTextEdit'i temizle

    if (searchText.isEmpty()) {
        // Arama metni boşsa, tüm logları göster
        for (const QString& log : allLogsBuffer) {
            logArea->append(log);
        }
    } else {
        // Arama metni varsa, sadece eşleşen logları göster
        for (const QString& log : allLogsBuffer) {
            if (log.contains(searchText, Qt::CaseInsensitive)) { // Büyük/küçük harf duyarsız arama
                logArea->append(log); 
            }
        }
    }

    // Her filtreleme/arama sonrası en alta kaydır
    logArea->verticalScrollBar()->setValue(logArea->verticalScrollBar()->maximum());
}