#ifndef LOGPANEL_H
#define LOGPANEL_H

#include <QWidget>
#include <QTextEdit> // QTextEdit'in tam tanımına LogPanel'in kendisi ihtiyaç duyduğu için
#include <QStringList> // updatePanel metodunun imzası için

#include "../engine_integration.h" // LogData ve GraphData için (mevcut include)

class LogPanel : public QWidget
{
    Q_OBJECT // Bu makro kalmalı
public:
    explicit LogPanel(QWidget* parent = nullptr);
    // updatePanel artık bir slot olmak zorunda değil, sadece bir public metot
    void updatePanel(const QStringList& logs);

    // YENİDEN ETKİNLEŞTİRİLDİ: QTextEdit'e erişim için getter metodu
    QTextEdit* getLogTextEdit() const { return logArea; }

private:
    QTextEdit* logArea; // LogPanel'in içerdiği QTextEdit objesi
};

#endif // LOGPANEL_H