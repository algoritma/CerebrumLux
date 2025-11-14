#ifndef CHAT_PANEL_H
#define CHAT_PANEL_H

#include <QWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollBar> // QTextEdit'in kaydırma çubuğu için
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDateTime> // QDateTime için

#include "../../core/logger.h" // LOG_DEFAULT makrosu için
#include "../../communication/natural_language_processor.h" // CerebrumLux::ChatResponse için
#include "../../core/enums.h" // LogLevel için

namespace CerebrumLux {

class ChatPanel : public QWidget
{
    Q_OBJECT
public:
    explicit ChatPanel(QWidget *parent = nullptr);
    virtual ~ChatPanel();

public slots:
    // Chat mesajlarını ve ChatResponse objesini ekler (SimulationPanel'den taşındı)
    void appendChatMessage(const QString& sender, const CerebrumLux::ChatResponse& chatResponse); 

signals:
    // Kullanıcı chat mesajı girdiğinde (MainWindow'a bağlanacak)
    void chatMessageEntered(const QString& message);

private slots:
    // Chat mesajı girişi için slot (SimulationPanel'den taşındı)
    void onChatMessageLineEditReturnPressed();

private:
    QTextEdit *chatHistoryDisplay;
    QLineEdit *chatMessageLineEdit;
    QPushButton *sendChatMessageButton;

    void setupUi();
};

} // namespace CerebrumLux

#endif // CHAT_PANEL_H