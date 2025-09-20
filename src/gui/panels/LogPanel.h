#ifndef LOGPANEL_H
#define LOGPANEL_H

#include <QWidget>
#include <QTextEdit> 
#include <QStringList> 
#include <QLineEdit> // YENİ: Arama kutusu için
#include <QVBoxLayout> // YENİ: Düzenleme için (eğer henüz yoksa)

#include "../engine_integration.h" 

class LogPanel : public QWidget
{
    Q_OBJECT 
public:
    explicit LogPanel(QWidget* parent = nullptr);
    void updatePanel(const QStringList& logs); // Mevcut metot

    QTextEdit* getLogTextEdit() const { return logArea; }

private slots:
    // YENİ: Arama kutusundaki metin değiştiğinde çağrılacak slot
    void onSearchTextChanged(const QString& searchText);

private:
    QTextEdit* logArea; 
    QLineEdit* searchLineEdit; // YENİ: Arama kutusu
    QStringList allLogsBuffer; // YENİ: Tüm logları tutacak dahili buffer (filtreleme için)
    QVBoxLayout* mainLayout; // YENİ: LogPanel'in ana düzeni
};

#endif // LOGPANEL_H