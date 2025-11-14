#include "ChatPanel.h"

namespace CerebrumLux {

ChatPanel::ChatPanel(QWidget *parent) : QWidget(parent)
{
    setupUi();
    LOG_DEFAULT(LogLevel::INFO, "ChatPanel: Initialized.");
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

    QHBoxLayout *chatInputLayout = new QHBoxLayout();
    chatMessageLineEdit = new QLineEdit(this);
    chatMessageLineEdit->setPlaceholderText("Buraya mesajınızı yazın...");
    chatInputLayout->addWidget(chatMessageLineEdit);

    sendChatMessageButton = new QPushButton("Gönder", this);
    chatInputLayout->addWidget(sendChatMessageButton);

    mainLayout->addLayout(chatInputLayout);

    connect(chatMessageLineEdit, &QLineEdit::returnPressed, this, &CerebrumLux::ChatPanel::onChatMessageLineEditReturnPressed);
    connect(sendChatMessageButton, &QPushButton::clicked, this, &CerebrumLux::ChatPanel::onChatMessageLineEditReturnPressed);

    // Chat geçmişinin otomatik aşağı kayması için
    connect(chatHistoryDisplay->verticalScrollBar(), &QScrollBar::rangeChanged,
            [this](int min, int max){ Q_UNUSED(min); chatHistoryDisplay->verticalScrollBar()->setValue(max); });

    setLayout(mainLayout);
}

void ChatPanel::appendChatMessage(const QString& sender, const CerebrumLux::ChatResponse& chatResponse) {
    // Chat geçmişini HTML olarak oluşturacağız
    QString messageHtml = chatHistoryDisplay->toHtml();
    
    // Zaman damgasını al
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");

    // Mesajı sender'a göre farklı renklendir ve HTML formatında ekle
    if (sender == "CerebrumLux") {
        messageHtml += QString("<p style=\"margin-bottom:0;\"><b><font color=\"#007bff\">%1 %2:</font></b> %3</p>")
                       .arg(timestamp).arg(sender).arg(QString::fromStdString(chatResponse.text));
        
        // Gerekçeyi daha küçük ve soluk renkte ekle (eğer varsa)
        if (!chatResponse.reasoning.empty()) {
            messageHtml += QString("<p style=\"margin-left:20px; font-size:0.8em; color:#6c757d; margin-top:0;\"><i>Gerekçe: %1</i></p>")
                           .arg(QString::fromStdString(chatResponse.reasoning));
        }
        // Açıklama gerekliyse bir uyarı ekle
        if (chatResponse.needs_clarification) {
            messageHtml += QString("<p style=\"margin-left:20px; font-size:0.8em; color:#ffc107; margin-top:0;\"><i>(Bu yanıt belirsiz olabilir, ek bilgi sağlamak ister misiniz?)</i></p>");
        }
    } else { // Kullanıcı mesajı
        messageHtml += QString("<p style=\"margin-bottom:0;\"><b><font color=\"#28a745\">%1 %2:</font></b> %3</p>")
                       .arg(timestamp).arg(sender).arg(QString::fromStdString(chatResponse.text));
    }

    chatHistoryDisplay->setHtml(messageHtml);

    // Otomatik olarak en alta kaydır
    QScrollBar *sb = chatHistoryDisplay->verticalScrollBar();
    if (sb) {
        sb->setValue(sb->maximum());
    }
}

void ChatPanel::onChatMessageLineEditReturnPressed() {
    LOG_DEFAULT(LogLevel::DEBUG, "ChatPanel: onChatMessageLineEditReturnPressed slot triggered.");
    QString message = chatMessageLineEdit->text();
    if (!message.isEmpty()) {
        // Kullanıcı mesajı için bir ChatResponse objesi oluştur
        CerebrumLux::ChatResponse userChatResponse;
        userChatResponse.text = message.toStdString();
        appendChatMessage("User", userChatResponse); // Yeni imzaya uygun çağrı
        emit chatMessageEntered(message);
        chatMessageLineEdit->clear();
    } else {
        LOG_DEFAULT(LogLevel::WARNING, "ChatPanel: Boş chat mesajı girildi.");
    }
}

} // namespace CerebrumLux