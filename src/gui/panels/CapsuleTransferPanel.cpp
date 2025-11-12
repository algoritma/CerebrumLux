#include "CapsuleTransferPanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QHeaderView>
#include "../../core/logger.h"
#include "../../core/utils.h"
#include "../../external/nlohmann/json.hpp"

namespace CerebrumLux {

CapsuleTransferPanel::CapsuleTransferPanel(QWidget *parent) : QWidget(parent)
{
    setupUi();
    LOG_DEFAULT(LogLevel::INFO, "CapsuleTransferPanel: Initialized.");
}

CapsuleTransferPanel::~CapsuleTransferPanel() { // 'override' anahtar kelimesi KALDIRILDI
    LOG_DEFAULT(LogLevel::INFO, "CapsuleTransferPanel: Destructor called.");
}

void CapsuleTransferPanel::setupUi() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    capsuleJsonLabel = new QLabel("Kapsül JSON:", this);
    capsuleJsonInput = new QTextEdit(this);
    capsuleJsonInput->setPlaceholderText("Buraya kapsül JSON'unu yapıştırın...");
    capsuleJsonInput->setMinimumHeight(100);

    signatureLabel = new QLabel("İmza (Base64):", this);
    signatureInput = new QLineEdit(this);
    signatureInput->setPlaceholderText("Kapsülün Ed25519 imzasını girin...");

    senderIdLabel = new QLabel("Gönderen ID:", this);
    senderIdInput = new QLineEdit(this);
    senderIdInput->setPlaceholderText("Gönderen AI'ın ID'si...");

    ingestCapsuleButton = new QPushButton("Kapsülü Enjekte Et", this);
    connect(ingestCapsuleButton, &QPushButton::clicked, this, &CerebrumLux::CapsuleTransferPanel::onIngestCapsuleClicked);

    webQueryLabel = new QLabel("Web Sorgusu/URL:", this);
    webQueryInput = new QLineEdit(this);
    webQueryInput->setPlaceholderText("Arama terimi veya URL girin (ör: Cerebrum Lux github)");
    fetchWebCapsuleButton = new QPushButton("Web'den Kapsül Çek", this);
    connect(fetchWebCapsuleButton, &QPushButton::clicked, this, &CerebrumLux::CapsuleTransferPanel::onFetchWebCapsuleClicked);

    reportStatusLabel = new QLabel("İşlem Raporu:", this);
    reportMessageDisplay = new QTextEdit(this);
    reportMessageDisplay->setReadOnly(true);
    reportMessageDisplay->setMinimumHeight(80);

    mainLayout->addWidget(capsuleJsonLabel);
    mainLayout->addWidget(capsuleJsonInput);
    mainLayout->addWidget(signatureLabel);
    mainLayout->addWidget(signatureInput);
    mainLayout->addWidget(senderIdLabel);
    mainLayout->addWidget(senderIdInput);
    mainLayout->addWidget(ingestCapsuleButton);
    mainLayout->addWidget(webQueryLabel);
    mainLayout->addWidget(webQueryInput);
    mainLayout->addWidget(fetchWebCapsuleButton);
    mainLayout->addWidget(reportStatusLabel);
    mainLayout->addWidget(reportMessageDisplay);

    setLayout(mainLayout);
}

void CapsuleTransferPanel::displayIngestReport(const IngestReport& report) {
    QString htmlReport;
    QString result_str;
    QString color;

    switch (report.result) {
        case IngestResult::Success:
            result_str = "BAŞARILI";
            color = "#28a745"; // Yeşil
            break;
        case IngestResult::InvalidSignature:
        case IngestResult::DecryptionFailed:
        case IngestResult::SchemaMismatch:
        case IngestResult::SteganographyDetected:
        case IngestResult::SandboxFailed:
        case IngestResult::CorroborationFailed:
        case IngestResult::UnknownError:
            result_str = "HATA";
            color = "#dc3545"; // Kırmızı
            break;
        case IngestResult::SanitizationNeeded:
            result_str = "UYARI (Temizleme Yapıldı)";
            color = "#ffc107"; // Sarı
            break;
        case IngestResult::Busy:
            result_str = "UYARI (Meşgul)";
            color = "#ffc107"; // Sarı
            break;
    }

    htmlReport += QString("<p style=\"margin-bottom:0;\"><b><font color=\"%1\">İşlem Sonucu: %2</font></b></p>").arg(color).arg(result_str);
    htmlReport += QString("<p style=\"margin-left:10px;\"><b>Mesaj:</b> %1</p>").arg(QString::fromStdString(report.message));
    htmlReport += QString("<p style=\"margin-left:10px;\"><b>Kaynak Eş ID:</b> %1</p>").arg(QString::fromStdString(report.source_peer_id));
    htmlReport += QString("<p style=\"margin-left:10px;\"><b>Orijinal Kapsül ID:</b> %1</p>").arg(QString::fromStdString(report.original_capsule.id));
    htmlReport += QString("<p style=\"margin-left:10px;\"><b>İşlenmiş İçerik (ilk 500 karakter):</b> %1...</p>").arg(QString::fromStdString(report.processed_capsule.content.substr(0, std::min((size_t)500, report.processed_capsule.content.length()))));
    reportMessageDisplay->setHtml(htmlReport);

    LogLevel level = (report.result == IngestResult::Success) ? LogLevel::INFO : LogLevel::ERR_CRITICAL;
    LOG_DEFAULT(level, "Kapsül Enjeksiyon Raporu: ID=" << report.original_capsule.id << ", Sonuç=" << static_cast<int>(report.result) << ", Mesaj=" << report.message);
}

void CapsuleTransferPanel::onIngestCapsuleClicked() {
    LOG_DEFAULT(LogLevel::INFO, "CapsuleTransferPanel: 'Kapsülü Enjekte Et' butonu tıklandı.");
    QString capsuleJson = capsuleJsonInput->toPlainText();
    QString signature = signatureInput->text();
    QString senderId = senderIdInput->text();
    emit ingestCapsuleRequest(capsuleJson, signature, senderId);
}

void CapsuleTransferPanel::onFetchWebCapsuleClicked() {
    LOG_DEFAULT(LogLevel::INFO, "CapsuleTransferPanel: 'Web'den Kapsül Çek' butonu tıklandı. Sorgu: " << webQueryInput->text().toStdString());
    emit fetchWebCapsuleRequest(webQueryInput->text());
    reportMessageDisplay->clear();
    reportMessageDisplay->setText("Web'den içerik çekiliyor...");
}

} // namespace CerebrumLux