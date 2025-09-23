#include "CapsuleTransferPanel.h"
#include <QDateTime> // IngestReport timestamp için
#include "../../core/logger.h" // Loglama için
#include <QDebug> // Debug çıktıları için

CapsuleTransferPanel::CapsuleTransferPanel(QWidget* parent) : QWidget(parent)
{
    mainLayout = new QVBoxLayout(this);

    // 1. JSON Giriş Alanı
    jsonInputTextEdit = new QTextEdit(this);
    jsonInputTextEdit->setPlaceholderText("Enter Capsule JSON here...");
    mainLayout->addWidget(jsonInputTextEdit);

    // 2. İmza ve Gönderen ID Alanları ile Ingest Butonu
    ingestControlsLayout = new QHBoxLayout();
    signatureLineEdit = new QLineEdit(this);
    signatureLineEdit->setPlaceholderText("Enter Signature (Base64)");
    senderIdLineEdit = new QLineEdit(this);
    senderIdLineEdit->setPlaceholderText("Enter Sender ID");
    ingestCapsuleButton = new QPushButton("Ingest Capsule", this);

    ingestControlsLayout->addWidget(signatureLineEdit);
    ingestControlsLayout->addWidget(senderIdLineEdit);
    ingestControlsLayout->addWidget(ingestCapsuleButton);
    mainLayout->addLayout(ingestControlsLayout);

    // 3. Web Sorgu Alanı ve Fetch Butonu
    fetchWebControlsLayout = new QHBoxLayout();
    webQueryLineEdit = new QLineEdit(this);
    webQueryLineEdit->setPlaceholderText("Enter Web Search Query for Capsule");
    fetchFromWebButton = new QPushButton("Fetch from Web", this);

    fetchWebControlsLayout->addWidget(webQueryLineEdit);
    fetchWebControlsLayout->addWidget(fetchFromWebButton);
    mainLayout->addLayout(fetchWebControlsLayout);

    // 4. Ingest Raporu Gösterim Alanı
    ingestReportTextEdit = new QTextEdit(this);
    ingestReportTextEdit->setReadOnly(true);
    ingestReportTextEdit->setPlaceholderText("Ingest Report will appear here...");
    mainLayout->addWidget(ingestReportTextEdit);

    setLayout(mainLayout);

    // Sinyal-Slot Bağlantıları
    connect(ingestCapsuleButton, &QPushButton::clicked, this, &CapsuleTransferPanel::onIngestCapsuleClicked);
    connect(fetchFromWebButton, &QPushButton::clicked, this, &CapsuleTransferPanel::onFetchFromWebClicked);

    // IngestReport'u Qt'nin meta sistemine kaydet (DataTypes.h'de zaten yapıldı)
    // qRegisterMetaType<IngestReport>("IngestReport"); // Zaten DataTypes.h içinde Q_DECLARE_METATYPE ile bildirilmişti
    // Ancak sinyal/slot bağlantılarında argüman olarak kullanılabilmesi için qRegisterMetaType<IngestReport>() çağrısı da uygun bir yerde yapılmalı.
    // main.cpp'de QApplication kurulduktan sonra veya MainWindow constructor'ında olabilir.
    
    LOG_DEFAULT(LogLevel::INFO, "CapsuleTransferPanel: Initialized.");
}

void CapsuleTransferPanel::onIngestCapsuleClicked()
{
    QString capsuleJson = jsonInputTextEdit->toPlainText();
    QString signature = signatureLineEdit->text();
    QString senderId = senderIdLineEdit->text();

    if (capsuleJson.isEmpty()) {
        ingestReportTextEdit->append(QDateTime::currentDateTime().toString("HH:mm:ss") + " [ERROR] Capsule JSON cannot be empty.");
        LOG_DEFAULT(LogLevel::WARNING, "CapsuleTransferPanel: Ingest Capsule JSON input is empty.");
        return;
    }

    LOG_DEFAULT(LogLevel::INFO, "CapsuleTransferPanel: Emitting ingestCapsuleRequest for sender: " << senderId.toStdString());
    emit ingestCapsuleRequest(capsuleJson, signature, senderId);
}

void CapsuleTransferPanel::onFetchFromWebClicked()
{
    QString query = webQueryLineEdit->text();

    if (query.isEmpty()) {
        ingestReportTextEdit->append(QDateTime::currentDateTime().toString("HH:mm:ss") + " [ERROR] Web query cannot be empty.");
        LOG_DEFAULT(LogLevel::WARNING, "CapsuleTransferPanel: Fetch from Web query input is empty.");
        return;
    }

    LOG_DEFAULT(LogLevel::INFO, "CapsuleTransferPanel: Emitting fetchWebCapsuleRequest for query: " << query.toStdString());
    emit fetchWebCapsuleRequest(query);
}

void CapsuleTransferPanel::displayIngestReport(const IngestReport& report)
{
    QString timestamp = QDateTime::fromSecsSinceEpoch(std::chrono::system_clock::to_time_t(report.timestamp)).toString("HH:mm:ss");
    QString reportMsg = QString("%1 [REPORT] Source: %2, ID: %3, Result: %4, Message: %5")
                            .arg(timestamp)
                            .arg(QString::fromStdString(report.source_peer_id))
                            .arg(QString::fromStdString(report.original_capsule.id))
                            .arg(static_cast<int>(report.result))
                            .arg(QString::fromStdString(report.message));
    
    // Processed capsule content'ini de ekleyelim
    if (report.result == IngestResult::Success || report.result == IngestResult::SanitizationNeeded) {
        reportMsg += "\n    Processed Content: " + QString::fromStdString(report.processed_capsule.content.substr(0, std::min((size_t)200, report.processed_capsule.content.length()))) + "...";
    }

    ingestReportTextEdit->append(reportMsg);
    LOG_DEFAULT(LogLevel::INFO, "CapsuleTransferPanel: Displaying ingest report for capsule ID: " << report.original_capsule.id);
}