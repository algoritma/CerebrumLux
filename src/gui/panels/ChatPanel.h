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
#include <QTextToSpeech> // YENİ: TTS Kütüphanesi

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
    // Mesaj ekleme fonksiyonunu güncelliyoruz (ChatResponse alacak şekilde)
    void appendChatMessage(const QString& sender, const CerebrumLux::ChatResponse& response);
    // Basit metin eklemek için overload (Kullanıcı mesajları için)
    void appendChatMessage(const QString& sender, const QString& message);

signals:
    // Kullanıcı chat mesajı girdiğinde (MainWindow'a bağlanacak)
    void chatMessageEntered(const QString& message);
    // YENİ: Öneri tıklandığında ve geri bildirim verildiğinde yayılacak sinyaller
    void suggestionClicked(const QString& message);
    void userFeedbackGiven(bool isPositive);

private slots:
    void onSendClicked();
    void onSuggestionBtnClicked();
    void onLikeClicked();
    void onDislikeClicked();
    void onToggleVoiceClicked(); // YENİ: Ses aç/kapa slotu

private:
    QTextEdit *chatHistoryDisplay;
    QLineEdit *chatMessageLineEdit;
    QPushButton *sendChatMessageButton;
    
    // YENİ UI Elemanları
    QWidget *suggestionContainer;
    QHBoxLayout *suggestionLayout;
    QPushButton *btnLike;
    QPushButton *btnDislike;
   
    // YENİ: Sesli Yanıt Elemanları
    QTextToSpeech *tts;
    QPushButton *btnVoiceToggle;
    bool isVoiceEnabled;

    void clearSuggestions();
    void addSuggestionButton(const std::string& text);
    void setupUi();
};

} // namespace CerebrumLux

#endif // CHAT_PANEL_H