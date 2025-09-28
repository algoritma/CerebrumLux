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

    mainLayout->addSpacing(20);
    mainLayout->addWidget(new QLabel("KnowledgeBase İçeriği:", this));

    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);

    capsuleListWidget = new QListWidget(this);
    capsuleListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(capsuleListWidget, &QListWidget::currentItemChanged, this, &CerebrumLux::CapsuleTransferPanel::onSelectedCapsuleChanged);
    splitter->addWidget(capsuleListWidget);

    capsuleDetailDisplay = new QTextEdit(this);
    capsuleDetailDisplay->setReadOnly(true);
    splitter->addWidget(capsuleDetailDisplay);

    mainLayout->addWidget(splitter);

    QList<int> sizes;
    sizes << width() * 0.3 << width() * 0.7;
    splitter->setSizes(sizes);

    setLayout(mainLayout);
}

void CapsuleTransferPanel::displayIngestReport(const IngestReport& report) {
    QString status = "İşlem Sonucu: " + QString::number(static_cast<int>(report.result)) + "\n";
    status += "Mesaj: " + QString::fromStdString(report.message) + "\n";
    status += "Kaynak Eş ID: " + QString::fromStdString(report.source_peer_id) + "\n";
    status += "Orijinal Kapsül ID: " + QString::fromStdString(report.original_capsule.id) + "\n";
    status += "    İşlenmiş İçerik (ilk 200 karakter): " + QString::fromStdString(report.processed_capsule.content.substr(0, std::min((size_t)200, report.processed_capsule.content.length()))) + "...\n";
    reportMessageDisplay->setText(status);

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

void CapsuleTransferPanel::updateCapsuleList(const std::vector<Capsule>& capsules) {
    LOG_DEFAULT(LogLevel::DEBUG, "CapsuleTransferPanel: Kapsül listesi güncelleniyor. Toplam kapsül: " << capsules.size());
    capsuleListWidget->clear();
    displayedCapsules.clear();

    for (const auto& capsule : capsules) {
        QString listItemText = QString("ID: %1 | Konu: %2 | Kaynak: %3")
                                .arg(QString::fromStdString(capsule.id))
                                .arg(QString::fromStdString(capsule.topic))
                                .arg(QString::fromStdString(capsule.source));
        
        QListWidgetItem *item = new QListWidgetItem(listItemText, capsuleListWidget);
        item->setData(Qt::UserRole, QString::fromStdString(capsule.id));

        CapsuleDisplayData data;
        data.id = QString::fromStdString(capsule.id);
        data.topic = QString::fromStdString(capsule.topic);
        data.source = QString::fromStdString(capsule.source);
        data.summary = QString::fromStdString(capsule.plain_text_summary);
        data.fullContent = QString::fromStdString(capsule.content);
        data.cryptofigBlob = QString::fromStdString(capsule.cryptofig_blob_base64);
        data.confidence = capsule.confidence;
        displayedCapsules[data.id] = data;
    }
    LOG_DEFAULT(LogLevel::DEBUG, "CapsuleTransferPanel: Kapsül listesi güncellendi.");
}

void CapsuleTransferPanel::onSelectedCapsuleChanged(QListWidgetItem* current, QListWidgetItem* previous) {
    Q_UNUSED(previous);
    if (!current) {
        capsuleDetailDisplay->clear();
        return;
    }

    QString selectedCapsuleId = current->data(Qt::UserRole).toString();
    auto it = displayedCapsules.find(selectedCapsuleId);
    if (it != displayedCapsules.end()) {
        displayCapsuleDetails(it->second);
        LOG_DEFAULT(LogLevel::DEBUG, "CapsuleTransferPanel: Kapsül detayları gösterildi. ID: " << selectedCapsuleId.toStdString());
    } else {
        capsuleDetailDisplay->setText("Detaylar bulunamadı.");
        LOG_DEFAULT(LogLevel::WARNING, "CapsuleTransferPanel: Seçilen kapsül ID'si dahili listeye uymuyor: " << selectedCapsuleId.toStdString());
    }
}

void CapsuleTransferPanel::displayCapsuleDetails(const CapsuleDisplayData& data) {
    QString details;
    details += "<h3>Kapsül Detayları</h3>";
    details += "<b>ID:</b> " + data.id + "<br>";
    details += "<b>Konu:</b> " + data.topic + "<br>";
    details += "<b>Kaynak:</b> " + data.source + "<br>";
    details += "<b>Güven Seviyesi:</b> " + QString::number(data.confidence, 'f', 2) + "<br>";
    details += "<b>Özet:</b> " + data.summary + "<br>";
    details += "<b>Cryptofig (Base64):</b> " + data.cryptofigBlob.left(100) + "...<br>";
    details += "<br><b>Tam İçerik:</b><br><pre>" + data.fullContent + "</pre>";

    capsuleDetailDisplay->setHtml(details);
}

} // namespace CerebrumLux