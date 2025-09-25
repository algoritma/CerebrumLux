#include "CapsuleTransferPanel.h"
#include "../../core/logger.h"
#include <QDateTime> // QDateTime için
#include <QFrame>    // QFrame için eklendi

namespace CerebrumLux {

CapsuleTransferPanel::CapsuleTransferPanel(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Kapsül Yutma Alanı
    mainLayout->addWidget(new QLabel("Kapsül JSON İçeriği:"));
    capsuleJsonInput = new QTextEdit(this);
    capsuleJsonInput->setPlaceholderText("Buraya kapsül JSON verisini yapıştırın...");
    mainLayout->addWidget(capsuleJsonInput);

    mainLayout->addWidget(new QLabel("İmza (Base64):"));
    signatureInput = new QLineEdit(this);
    signatureInput->setPlaceholderText("Buraya kapsül imzasını yapıştırın...");
    mainLayout->addWidget(signatureInput);

    mainLayout->addWidget(new QLabel("Gönderen ID'si:"));
    senderIdInput = new QLineEdit(this);
    senderIdInput->setPlaceholderText("Kapsülün geldiği eşin ID'si (örneğin 'Test_Peer_A')...");
    mainLayout->addWidget(senderIdInput);

    ingestButton = new QPushButton("Kapsülü Yut", this);
    mainLayout->addWidget(ingestButton);
    connect(ingestButton, &QPushButton::clicked, this, &CapsuleTransferPanel::onIngestButtonClicked);

    // Ayırıcı ekle
    QFrame* separator1 = new QFrame(this); // Yeni ayırıcı
    separator1->setFrameShape(QFrame::HLine);
    separator1->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(separator1);

    // Web'den Kapsül Çekme Alanı
    mainLayout->addWidget(new QLabel("Web'den Kapsül Çek:"));
    webQueryInput = new QLineEdit(this);
    webQueryInput->setPlaceholderText("Web'de aranacak anahtar kelimeler...");
    mainLayout->addWidget(webQueryInput);

    fetchWebButton = new QPushButton("Web'den Çek", this);
    mainLayout->addWidget(fetchWebButton);
    connect(fetchWebButton, &QPushButton::clicked, this, &CapsuleTransferPanel::onFetchWebButtonClicked);

    // Ayırıcı ekle
    QFrame* separator2 = new QFrame(this); // Yeni ayırıcı
    separator2->setFrameShape(QFrame::HLine);
    separator2->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(separator2);

    // Rapor Çıktısı Alanı
    mainLayout->addWidget(new QLabel("İşlem Raporu:"));
    reportOutput = new QTextEdit(this);
    reportOutput->setReadOnly(true);
    mainLayout->addWidget(reportOutput);

    setLayout(mainLayout);

    LOG_DEFAULT(LogLevel::INFO, "CapsuleTransferPanel: Initialized.");
}

void CapsuleTransferPanel::displayIngestReport(const IngestReport& report) {
    QString timestamp = QDateTime::fromSecsSinceEpoch(std::chrono::system_clock::to_time_t(report.timestamp)).toString("HH:mm:ss");
    QString reportMsg = QString("[%1] İşlem Sonucu: %2\nMesaj: %3\nKaynak Eş ID: %4\nOrijinal Kapsül ID: %5")
                            .arg(timestamp)
                            .arg(static_cast<int>(report.result))
                            .arg(QString::fromStdString(report.message))
                            .arg(QString::fromStdString(report.source_peer_id))
                            .arg(QString::fromStdString(report.original_capsule.id));

    if (report.result == IngestResult::Success || report.result == IngestResult::SanitizationNeeded) {
        reportMsg += "\n    İşlenmiş İçerik (ilk 200 karakter): " + QString::fromStdString(report.processed_capsule.content.substr(0, std::min((size_t)200, report.processed_capsule.content.length()))) + "...";
    }

    reportOutput->append(reportMsg);
    LOG_DEFAULT(LogLevel::INFO, "CapsuleTransferPanel: Displaying ingest report for Capsule ID: " << report.original_capsule.id);
}

void CapsuleTransferPanel::onIngestButtonClicked() {
    QString capsuleJson = capsuleJsonInput->toPlainText();
    QString signature = signatureInput->text();
    QString senderId = senderIdInput->text();

    if (capsuleJson.isEmpty() || signature.isEmpty() || senderId.isEmpty()) {
        reportOutput->append("<font color='red'>Hata: Tüm alanlar doldurulmalıdır (Kapsül JSON, İmza, Gönderen ID'si).</font>");
        return;
    }

    emit ingestCapsuleRequest(capsuleJson, signature, senderId);
    LOG_DEFAULT(LogLevel::INFO, "CapsuleTransferPanel: Ingest button clicked.");
}

void CapsuleTransferPanel::onFetchWebButtonClicked() {
    QString query = webQueryInput->text();
    if (query.isEmpty()) {
        reportOutput->append("<font color='red'>Hata: Web sorgusu boş olamaz.</font>");
        return;
    }
    emit fetchWebCapsuleRequest(query);
    LOG_DEFAULT(LogLevel::INFO, "CapsuleTransferPanel: Fetch Web button clicked for query: " << query.toStdString());
}

} // namespace CerebrumLux