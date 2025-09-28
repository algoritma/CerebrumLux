#include "KnowledgeBasePanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QHeaderView>
#include <QRegularExpression>
#include <QLabel> // QLabel'in tam tanımı için eklendi
#include "../../core/logger.h"
#include "../../core/utils.h"

namespace CerebrumLux {

KnowledgeBasePanel::KnowledgeBasePanel(LearningModule& learningModuleRef, QWidget *parent)
    : QWidget(parent),
      learningModule(learningModuleRef)
{
    setupUi();
    LOG_DEFAULT(LogLevel::INFO, "KnowledgeBasePanel: Initialized.");
    updateKnowledgeBaseContent(); // Başlangıçta içeriği yükle
}

KnowledgeBasePanel::~KnowledgeBasePanel() {
    LOG_DEFAULT(LogLevel::INFO, "KnowledgeBasePanel: Destructor called.");
}

void KnowledgeBasePanel::setupUi() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0); // Layout'un kenar boşluklarını sıfırla - YENİ

    // Başlık
    mainLayout->addWidget(new QLabel("KnowledgeBase İçeriği:", this));

    // Arama kutusu ve temizle butonu
    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchLineEdit = new QLineEdit(this);
    searchLineEdit->setPlaceholderText("Kapsüllerde ara (ID, Konu, Özet)...");
    connect(searchLineEdit, &QLineEdit::textChanged, this, &CerebrumLux::KnowledgeBasePanel::onSearchTextChanged);
    
    clearSearchButton = new QPushButton("Temizle", this);
    connect(clearSearchButton, &QPushButton::clicked, this, &CerebrumLux::KnowledgeBasePanel::onClearSearchClicked);

    searchLayout->addWidget(searchLineEdit);
    searchLayout->addWidget(clearSearchButton);
    mainLayout->addLayout(searchLayout);

    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setStretchFactor(0, 1); // Liste için esneklik - YENİ
    splitter->setStretchFactor(1, 2); // Detaylar için daha fazla esneklik - YENİ

    // Kapsül Listesi
    capsuleListWidget = new QListWidget(this);
    capsuleListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(capsuleListWidget, &QListWidget::currentItemChanged, this, &CerebrumLux::KnowledgeBasePanel::onSelectedCapsuleChanged);
    splitter->addWidget(capsuleListWidget);

    // Kapsül Detayları
    capsuleDetailDisplay = new QTextEdit(this);
    capsuleDetailDisplay->setReadOnly(true);
    splitter->addWidget(capsuleDetailDisplay);

    mainLayout->addWidget(splitter); // Splitter'ı ana layout'a ekle

    mainLayout->addStretch(1); // En alta esnek boşluk ekle - YENİ

    setLayout(mainLayout);
}

void KnowledgeBasePanel::updateKnowledgeBaseContent() {
    LOG_DEFAULT(LogLevel::DEBUG, "KnowledgeBasePanel: KnowledgeBase içeriği güncelleniyor.");
    
    // Seçili kapsülün ID'sini kaydet
    QString selectedCapsuleId;
    if (capsuleListWidget->currentItem()) {
        selectedCapsuleId = capsuleListWidget->currentItem()->data(Qt::UserRole).toString();
        LOG_DEFAULT(LogLevel::TRACE, "KnowledgeBasePanel: Mevcut secili kapsul ID: " << selectedCapsuleId.toStdString());
    }

    currentDisplayedCapsules = learningModule.getKnowledgeBase().get_all_capsules(); // Tüm kapsülleri al
    filterAndDisplayCapsules(searchLineEdit->text()); // Mevcut filtreyle yeniden listele

    // Seçimi geri yükle
    if (!selectedCapsuleId.isEmpty()) {
        QListWidgetItem *itemToSelect = nullptr;
        for (int i = 0; i < capsuleListWidget->count(); ++i) {
            QListWidgetItem *item = capsuleListWidget->item(i);
            if (item->data(Qt::UserRole).toString() == selectedCapsuleId) {
                itemToSelect = item;
                break;
            }
        }
        if (itemToSelect) {
            capsuleListWidget->setCurrentItem(itemToSelect);
            LOG_DEFAULT(LogLevel::TRACE, "KnowledgeBasePanel: Secim geri yuklendi. ID: " << selectedCapsuleId.toStdString());
        } else {
            // Eğer önceki seçili kapsül artık listede yoksa, detayları temizle
            capsuleDetailDisplay->clear();
            LOG_DEFAULT(LogLevel::TRACE, "KnowledgeBasePanel: Onceki secili kapsul listede bulunamadi, detaylar temizlendi.");
        }
    } else {
        capsuleDetailDisplay->clear(); // Hiçbir şey seçili değilse detayları temizle
    }

    LOG_DEFAULT(LogLevel::DEBUG, "KnowledgeBasePanel: KnowledgeBase içeriği güncellendi. Toplam kapsül: " << currentDisplayedCapsules.size());
}

void KnowledgeBasePanel::onSelectedCapsuleChanged(QListWidgetItem* current, QListWidgetItem* previous) {
    Q_UNUSED(previous);
    if (!current) {
        capsuleDetailDisplay->clear();
        return;
    }

    QString selectedCapsuleId = current->data(Qt::UserRole).toString();
    auto it = displayedCapsuleDetails.find(selectedCapsuleId);
    if (it != displayedCapsuleDetails.end()) { // displayedCapsules yerine displayedCapsuleDetails olmalı
        displayCapsuleDetails(it->second);
        LOG_DEFAULT(LogLevel::DEBUG, "KnowledgeBasePanel: Kapsül detayları gösterildi. ID: " << selectedCapsuleId.toStdString());
    } else {
        capsuleDetailDisplay->setText("Detaylar bulunamadı.");
        LOG_DEFAULT(LogLevel::WARNING, "KnowledgeBasePanel: Seçilen kapsül ID'si dahili listeye uymuyor: " << selectedCapsuleId.toStdString());
    }
}

void KnowledgeBasePanel::onSearchTextChanged(const QString& text) {
    filterAndDisplayCapsules(text);
}

void KnowledgeBasePanel::onClearSearchClicked() {
    searchLineEdit->clear();
    filterAndDisplayCapsules("");
}

void KnowledgeBasePanel::filterAndDisplayCapsules(const QString& filterText) {
    capsuleListWidget->clear();
    displayedCapsuleDetails.clear(); // Detayları da temizle

    for (const auto& capsule : currentDisplayedCapsules) {
        QString capsuleId = QString::fromStdString(capsule.id);
        QString capsuleTopic = QString::fromStdString(capsule.topic);
        QString capsuleSource = QString::fromStdString(capsule.source); // Kaynak için de arama yapabiliriz
        QString capsuleSummary = QString::fromStdString(capsule.plain_text_summary);
        
        if (filterText.isEmpty() ||
            capsuleId.contains(filterText, Qt::CaseInsensitive) ||
            capsuleTopic.contains(filterText, Qt::CaseInsensitive) ||
            capsuleSource.contains(filterText, Qt::CaseInsensitive) || // Kaynakta da arama
            capsuleSummary.contains(filterText, Qt::CaseInsensitive))
        {
            QString listItemText = QString("ID: %1 | Konu: %2 | Kaynak: %3")
                                    .arg(capsuleId)
                                    .arg(capsuleTopic)
                                    .arg(QString::fromStdString(capsule.source));
            
            QListWidgetItem *item = new QListWidgetItem(listItemText, capsuleListWidget);
            item->setData(Qt::UserRole, capsuleId);

            // Detayları kolay erişim için dahili map'te sakla
            KnowledgeCapsuleDisplayData data;
            data.id = capsuleId;
            data.topic = capsuleTopic;
            data.source = QString::fromStdString(capsule.source);
            data.summary = capsuleSummary;
            data.fullContent = QString::fromStdString(capsule.content);
            data.cryptofigBlob = QString::fromStdString(capsule.cryptofig_blob_base64);
            data.confidence = capsule.confidence;
            displayedCapsuleDetails[data.id] = data;
        }
    }
    LOG_DEFAULT(LogLevel::DEBUG, "KnowledgeBasePanel: Kapsül listesi filtrelendi. Görüntülenen kapsül sayısı: " << capsuleListWidget->count());
    // capsuleDetailDisplay->clear(); // Seçili kapsül detayı da temizlensin (updateKnowledgeBaseContent() içinde hallediliyor)
}

void KnowledgeBasePanel::displayCapsuleDetails(const KnowledgeCapsuleDisplayData& data) {
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