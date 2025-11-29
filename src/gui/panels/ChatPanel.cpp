#include <QLocale> // Dil ayarı için
#include <QRegularExpression> // Dil algılama için

#include "ChatPanel.h"

namespace CerebrumLux {

ChatPanel::ChatPanel(QWidget *parent) : QWidget(parent)
{
    setupUi();
    LOG_DEFAULT(LogLevel::INFO, "ChatPanel: Initialized.");

    // YENİ: TTS Motorunu Başlat
    tts = new QTextToSpeech(this);
    
    /*
    // Türkçe dilini ayarlamaya çalış
    QList<QLocale> locales = tts->availableLocales();
    for (const QLocale &locale : locales) {
        if (locale.language() == QLocale::Turkish) {
            tts->setLocale(locale);
            break;
        }
    }
    */
    // Başlangıçta varsayılan sistemi kullan, her konuşmada dinamik ayarlanacak.
  
    // Varsayılan olarak ses kapalı olsun (Kullanıcı isterse açsın)
    isVoiceEnabled = false; 

}

ChatPanel::~ChatPanel() {
    LOG_DEFAULT(LogLevel::INFO, "ChatPanel: Destructor called.");
}

void ChatPanel::setupUi() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0); // Kenar boşluklarını sıfırla

    mainLayout->addWidget(new QLabel("Chat Geçmişi:", this));

    chatHistoryDisplay = new QTextEdit(this);
    chatHistoryDisplay->setReadOnly(true);
    chatHistoryDisplay->setMinimumHeight(150);
    mainLayout->addWidget(chatHistoryDisplay);
    
    // YENİ: Öneri Butonları Alanı
    suggestionContainer = new QWidget(this);
    suggestionLayout = new QHBoxLayout(suggestionContainer);
    suggestionLayout->setContentsMargins(0, 0, 0, 0);
    suggestionLayout->addStretch(); // Butonları sola dayamak için
    mainLayout->addWidget(suggestionContainer);

    // Alt Panel (Input + Send + Feedback)
    QHBoxLayout *bottomLayout = new QHBoxLayout();
    
    chatMessageLineEdit = new QLineEdit(this);
    chatMessageLineEdit->setPlaceholderText("Mesajınızı yazın...");
    connect(chatMessageLineEdit, &QLineEdit::returnPressed, this, &CerebrumLux::ChatPanel::onSendClicked);
    
    sendChatMessageButton = new QPushButton("Gönder", this);
    connect(sendChatMessageButton, &QPushButton::clicked, this, &CerebrumLux::ChatPanel::onSendClicked);
    
    // YENİ: Feedback Butonları
    btnLike = new QPushButton("👍", this);
    btnLike->setToolTip("Bu yanıtı beğendim");
    btnLike->setFixedWidth(30);
    connect(btnLike, &QPushButton::clicked, this, &CerebrumLux::ChatPanel::onLikeClicked);

    btnDislike = new QPushButton("👎", this);
    btnDislike->setToolTip("Bu yanıtı beğenmedim");
    btnDislike->setFixedWidth(30);
    connect(btnDislike, &QPushButton::clicked, this, &CerebrumLux::ChatPanel::onDislikeClicked);
    
    // YENİ: Ses Aç/Kapa Butonu
    btnVoiceToggle = new QPushButton("🔇", this); // Başlangıçta sessiz ikonu
    btnVoiceToggle->setToolTip("Sesli Yanıtı Aç/Kapat");
    btnVoiceToggle->setFixedWidth(30);
    btnVoiceToggle->setCheckable(true);
    connect(btnVoiceToggle, &QPushButton::clicked, this, &CerebrumLux::ChatPanel::onToggleVoiceClicked);

    // Başlangıçta feedback butonları pasif olabilir veya aktif kalabilir
    
    bottomLayout->addWidget(chatMessageLineEdit);
    bottomLayout->addWidget(sendChatMessageButton);
    bottomLayout->addWidget(btnLike);
    bottomLayout->addWidget(btnDislike);
    bottomLayout->addWidget(btnVoiceToggle);

    mainLayout->addLayout(bottomLayout);

    // Chat geçmişinin otomatik aşağı kayması için
    connect(chatHistoryDisplay->verticalScrollBar(), &QScrollBar::rangeChanged,
            [this](int min, int max){ Q_UNUSED(min); chatHistoryDisplay->verticalScrollBar()->setValue(max); });

    setLayout(mainLayout);
}

void ChatPanel::appendChatMessage(const QString& sender, const QString& message) {
    chatHistoryDisplay->append(QString("<b>%1:</b> %2").arg(sender, message));
}

void ChatPanel::appendChatMessage(const QString& sender, const CerebrumLux::ChatResponse& response) {
    // Chat geçmişini HTML olarak oluşturacağız
    // 1. Metni Göster
    QString formattedMessage = QString("<b>%1:</b> %2").arg(sender, QString::fromStdString(response.text));
    
    // Gerekçe varsa ekle (debug modunda veya isteğe bağlı)
    if (!response.reasoning.empty()) {
        formattedMessage += QString("<br><i><small>(Gerekçe: %1)</small></i>").arg(QString::fromStdString(response.reasoning));
    }

    chatHistoryDisplay->append(formattedMessage);

    // YENİ: Sesli Okuma (Eğer aktifse ve gönderen CerebrumLux ise)
    if (isVoiceEnabled && sender == "CerebrumLux") {
        // HTML etiketlerini temizle (Sadece metni oku)
        QTextDocument doc;
        doc.setHtml(QString::fromStdString(response.text));
        QString plainText = doc.toPlainText();

        // --- DİNAMİK DİL ALGILAMA ---
        // Metin Türkçe karakterler içeriyor mu? (ı, ğ, ü, ş, ö, ç ve büyük halleri) (Language enum'u artık core/enums.h'den geliyor)
        // Eğer içeriyorsa Türkçe motoru, içermiyorsa İngilizce motoru seç.
        QLocale::Language targetLang = QLocale::English; // Varsayılan İngilizce
        QRegularExpression trRegex(QString::fromUtf8("[ığüşöçİĞÜŞÖÇ]"));
        
        if (plainText.contains(trRegex)) {
            targetLang = QLocale::Turkish;
        }

        // TTS Motorunu uygun dile ayarla
        QList<QLocale> locales = tts->availableLocales();
        for (const QLocale &locale : locales) {
            if (locale.language() == targetLang) {
                tts->setLocale(locale);
                break;
            }
        }
        // ---------------------------

        // Okumayı başlat
        if (tts->state() == QTextToSpeech::Speaking) tts->stop();
        tts->say(plainText);
    }

    // 2. Önerileri Göster
    clearSuggestions(); // Öncekileri temizle
    for (const auto& suggestion : response.suggested_questions) {
        addSuggestionButton(suggestion);
    }

    // Otomatik olarak en alta kaydır
    QScrollBar *sb = chatHistoryDisplay->verticalScrollBar();
    if (sb) {
        sb->setValue(sb->maximum());
    }
}

void ChatPanel::addSuggestionButton(const std::string& text) {
    QPushButton* btn = new QPushButton(QString::fromStdString(text), this);
    btn->setStyleSheet("text-align: left; padding: 5px;");
    btn->setCursor(Qt::PointingHandCursor);
    connect(btn, &QPushButton::clicked, this, &CerebrumLux::ChatPanel::onSuggestionBtnClicked);
    
    // Stretch item'dan önceye ekle (layout'un son elemanı stretch)
    suggestionLayout->insertWidget(suggestionLayout->count() - 1, btn);
}

void ChatPanel::clearSuggestions() {
    QLayoutItem *item;
    // Sadece butonları sil, stretch item'ı koru (veya tamamen temizleyip stretch'i tekrar ekle)
    while ((item = suggestionLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }
    suggestionLayout->addStretch(); // Stretch'i geri koy
}

void ChatPanel::onSuggestionBtnClicked() {
    QPushButton* btn = qobject_cast<QPushButton*>(sender());
    if (btn) {
        QString text = btn->text();
        // Öneri tıklandığında bunu sanki kullanıcı yazmış gibi işle
        clearSuggestions();
        appendChatMessage("User", text);
        emit chatMessageEntered(text); 
    }
}

void ChatPanel::onLikeClicked() {
    emit userFeedbackGiven(true);
    // Görsel geri bildirim (isteğe bağlı)
    chatHistoryDisplay->append("<i><small>Geri bildiriminiz için teşekkürler (+)</small></i>");
}

void ChatPanel::onDislikeClicked() {
    emit userFeedbackGiven(false);
    chatHistoryDisplay->append("<i><small>Geri bildiriminiz için teşekkürler (-)</small></i>");
}

void ChatPanel::onSendClicked() {
    LOG_DEFAULT(LogLevel::DEBUG, "ChatPanel: onChatMessageLineEditReturnPressed slot triggered.");
    QString message = chatMessageLineEdit->text().trimmed();
    if (!message.isEmpty()) {
        // Kullanıcı yeni bir şey yazdığında eski önerileri temizle
        clearSuggestions();
        appendChatMessage("User", message);
        emit chatMessageEntered(message);
        chatMessageLineEdit->clear();
    } else {
        LOG_DEFAULT(LogLevel::WARNING, "ChatPanel: Boş chat mesajı girildi.");
    }
}

// YENİ: Ses butonu tıklama işlemi
void ChatPanel::onToggleVoiceClicked() {
    isVoiceEnabled = btnVoiceToggle->isChecked();
    if (isVoiceEnabled) {
        btnVoiceToggle->setText("🔊"); // Sesli ikon
    } else {
        btnVoiceToggle->setText("🔇"); // Sessiz ikon
        tts->stop(); // Eğer konuşuyorsa sustur
    }
}

} // namespace CerebrumLux