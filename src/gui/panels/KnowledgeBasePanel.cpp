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
    mainLayout->setContentsMargins(0, 0, 0, 0);

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

    // YENİ KOD: Filtreleme kontrolleri
    QHBoxLayout *filterLayout = new QHBoxLayout();

    filterLayout->addWidget(new QLabel("Konu:", this));
    topicFilterComboBox = new QComboBox(this);
    topicFilterComboBox->addItem("Tümü"); // Tüm konuları göster seçeneği
    connect(topicFilterComboBox, &QComboBox::currentTextChanged, this, &CerebrumLux::KnowledgeBasePanel::onTopicFilterChanged);
    filterLayout->addWidget(topicFilterComboBox);

    filterLayout->addWidget(new QLabel("Başlangıç Tarihi:", this));
    // DÜZELTME: Daha geniş bir varsayılan tarih aralığı
    startDateEdit = new QDateEdit(QDate(2000, 1, 1), this); // Çok eski bir başlangıç tarihi
    startDateEdit->setCalendarPopup(true);
    connect(startDateEdit, &QDateEdit::dateChanged, this, &CerebrumLux::KnowledgeBasePanel::onStartDateChanged);
    filterLayout->addWidget(startDateEdit);

    filterLayout->addWidget(new QLabel("Bitiş Tarihi:", this));
    // DÜZELTME: Gelecekteki bir tarihi varsayılan bitiş tarihi yapalım
    endDateEdit = new QDateEdit(QDate::currentDate().addYears(1), this); // Bugün + 1 yıl
    endDateEdit->setCalendarPopup(true);
    connect(endDateEdit, &QDateEdit::dateChanged, this, &CerebrumLux::KnowledgeBasePanel::onEndDateChanged);
    filterLayout->addWidget(endDateEdit);

    filterLayout->addStretch();

    mainLayout->addLayout(filterLayout);

    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);

    capsuleListWidget = new QListWidget(this);
    capsuleListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(capsuleListWidget, &QListWidget::currentItemChanged, this, &CerebrumLux::KnowledgeBasePanel::onSelectedCapsuleChanged);
    splitter->addWidget(capsuleListWidget);

    capsuleDetailDisplay = new QTextEdit(this);
    capsuleDetailDisplay->setReadOnly(true);
    splitter->addWidget(capsuleDetailDisplay);

    // YENİ KOD: Geri bildirim butonları
    QHBoxLayout *feedbackLayout = new QHBoxLayout();
    acceptSuggestionButton = new QPushButton("Öneriyi Kabul Et", this);
    rejectSuggestionButton = new QPushButton("Öneriyi Reddet", this);
    feedbackLayout->addWidget(acceptSuggestionButton);
    feedbackLayout->addWidget(rejectSuggestionButton);
    mainLayout->addLayout(feedbackLayout);

    connect(acceptSuggestionButton, &QPushButton::clicked, this, &CerebrumLux::KnowledgeBasePanel::onAcceptSuggestionClicked);
    connect(rejectSuggestionButton, &QPushButton::clicked, this, &CerebrumLux::KnowledgeBasePanel::onRejectSuggestionClicked);
    // Başlangıçta butonları pasif yap
    updateSuggestionFeedbackButtons("");

    mainLayout->addWidget(splitter);

    mainLayout->addStretch(1);

    setLayout(mainLayout);
}

void KnowledgeBasePanel::updateKnowledgeBaseContent() {
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "KnowledgeBasePanel: KnowledgeBase içeriği güncelleniyor.");

    QString selectedCapsuleId;
    if (capsuleListWidget->currentItem()) {
        selectedCapsuleId = capsuleListWidget->currentItem()->data(Qt::UserRole).toString();
        LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "KnowledgeBasePanel: Mevcut secili kapsul ID: " << selectedCapsuleId.toStdString());
    }

    currentDisplayedCapsules = learningModule.getKnowledgeBase().get_all_capsules(); // Tüm kapsülleri al

    // YENİ KOD: Konu filtreleme ComboBox'ını doldur ve varsayılan değerleri ayarla
    QSet<QString> uniqueTopics;
    for (const auto& capsule : currentDisplayedCapsules) {
        uniqueTopics.insert(QString::fromStdString(capsule.topic));
    }
    topicFilterComboBox->blockSignals(true); // Sinyalleri geçici olarak engelle
    topicFilterComboBox->clear();
    topicFilterComboBox->addItem("Tümü");
    for (const QString& topic : uniqueTopics) {
        topicFilterComboBox->addItem(topic);
    }
    topicFilterComboBox->blockSignals(false); // Sinyalleri tekrar etkinleştir

    // Mevcut filtrelerle listeyi yeniden doldur
    filterAndDisplayCapsules(searchLineEdit->text(), topicFilterComboBox->currentText(), startDateEdit->date(), endDateEdit->date());

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
            LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "KnowledgeBasePanel: Secim geri yuklendi. ID: " << selectedCapsuleId.toStdString());
        } else {
            capsuleDetailDisplay->clear();
            LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "KnowledgeBasePanel: Onceki secili kapsul listede bulunamadi, detaylar temizlendi.");
        }
    } else {
        capsuleDetailDisplay->clear();
    }

    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "KnowledgeBasePanel: KnowledgeBase içeriği güncellendi. Toplam kapsül: " << currentDisplayedCapsules.size());
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
        updateSuggestionFeedbackButtons(""); // Kapsül bulunamazsa butonları pasif yap
        capsuleDetailDisplay->setText("Detaylar bulunamadı.");
        LOG_DEFAULT(LogLevel::WARNING, "KnowledgeBasePanel: Seçilen kapsül ID'si dahili listeye uymuyor: " << selectedCapsuleId.toStdString());
    }
}

void KnowledgeBasePanel::onSearchTextChanged(const QString& text) {
    // Mevcut filtreleri koruyarak arama filtresini uygula
    filterAndDisplayCapsules(text, topicFilterComboBox->currentText(), startDateEdit->date(), endDateEdit->date());
}

void KnowledgeBasePanel::onClearSearchClicked() {
    searchLineEdit->clear();
    // Mevcut filtreleri koruyarak arama filtresini temizle
    filterAndDisplayCapsules(QString(), topicFilterComboBox->currentText(), startDateEdit->date(), endDateEdit->date());
}

// YENİ SLOTLARIN İMPLEMENTASYONLARI
void KnowledgeBasePanel::onTopicFilterChanged(const QString& topic) {
    filterAndDisplayCapsules(searchLineEdit->text(), topic, startDateEdit->date(), endDateEdit->date());
}

void KnowledgeBasePanel::onStartDateChanged(const QDate& date) {
    // Düzeltme: Güncel topicFilterComboBox değeri ile çağrı
    filterAndDisplayCapsules(searchLineEdit->text(), topicFilterComboBox->currentText(), date, endDateEdit->date());
}

void KnowledgeBasePanel::onEndDateChanged(const QDate& date) {
    filterAndDisplayCapsules(searchLineEdit->text(), topicFilterComboBox->currentText(), startDateEdit->date(), date);
}

void KnowledgeBasePanel::filterAndDisplayCapsules(const QString& filterText, const QString& topicFilter, const QDate& startDate, const QDate& endDate) {
    capsuleListWidget->clear();
    displayedCapsuleDetails.clear();

    for (const auto& capsule : currentDisplayedCapsules) {
        QString capsuleId = QString::fromStdString(capsule.id);
        QString capsuleTopic = QString::fromStdString(capsule.topic);
        QString capsuleSource = QString::fromStdString(capsule.source);
        QString capsuleSummary = QString::fromStdString(capsule.plain_text_summary);
        QDateTime capsuleDateTime = QDateTime::fromSecsSinceEpoch(capsule.timestamp_utc.time_since_epoch().count());
        QDate capsuleDate = capsuleDateTime.date();

        bool textMatches = filterText.isEmpty() ||
                           capsuleId.contains(filterText, Qt::CaseInsensitive) ||
                           capsuleTopic.contains(filterText, Qt::CaseInsensitive) ||
                           capsuleSource.contains(filterText, Qt::CaseInsensitive) ||
                           capsuleSummary.contains(filterText, Qt::CaseInsensitive);

        bool topicMatches = topicFilter.isEmpty() || topicFilter == "Tümü" || capsuleTopic.compare(topicFilter, Qt::CaseInsensitive) == 0;
        
        bool dateMatches = (!startDate.isValid() || capsuleDate >= startDate) &&
                           (!endDate.isValid() || capsuleDate <= endDate);

        // DÜZELTME BAŞLANGICI: Filtre koşulunu kaldırarak tüm kapsüllerin eklenmesini sağla
        // if (textMatches && topicMatches && dateMatches) // Bu satır yorum satırı yapıldı veya kaldırıldı
        // DÜZELTME SONU
        { // Tüm kapsüllerin listeye eklenmesini sağla (geçici olarak filtrelemeyi devre dışı bırak)

            QString listItemText = QString("ID: %1 | Konu: %2 | Kaynak: %3 | Tarih: %4")
                                    .arg(capsuleId)
                                    .arg(capsuleTopic)
                                    .arg(QString::fromStdString(capsule.source))
                                    .arg(capsuleDateTime.toString("dd.MM.yyyy hh:mm"));
            
            QListWidgetItem *item = new QListWidgetItem(listItemText, capsuleListWidget);
            item->setData(Qt::UserRole, capsuleId);

            KnowledgeCapsuleDisplayData data;
            data.id = capsuleId;
            data.topic = capsuleTopic;
            data.source = QString::fromStdString(capsule.source);
            data.summary = capsuleSummary;
            data.fullContent = QString::fromStdString(capsule.content); // Corrected
            data.cryptofigBlob = QString::fromStdString(capsule.cryptofig_blob_base64); // Corrected
            data.confidence = capsule.confidence; // Corrected
            displayedCapsuleDetails[data.id] = data;
            // YENİ: Seçim yapıldığında butonları güncelle
        }
    }
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "KnowledgeBasePanel: Kapsül listesi filtrelendi. Görüntülenen kapsül sayısı: " << capsuleListWidget->count());
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
    // YENİ: Detaylar gösterildiğinde butonları güncelle
    updateSuggestionFeedbackButtons(data.id);
}

// YENİ METOT: Geri bildirim butonlarının durumunu günceller
void CerebrumLux::KnowledgeBasePanel::updateSuggestionFeedbackButtons(const QString& selectedCapsuleId) {
    bool enableButtons = false;
    if (!selectedCapsuleId.isEmpty()) {
        auto it = displayedCapsuleDetails.find(selectedCapsuleId);
        if (it != displayedCapsuleDetails.end()) {
            // Sadece "Kod Geliştirme Önerisi" (CodeDevelopmentSuggestion) tipindeki kapsüller için butonları aktif et
            // Capsule struct'ında InsightType üyesi olmadığı için bu kontrolü KnowledgeCapsuleDisplayData üzerinden yapamayız.
            // Bunun yerine KnowledgeBase'den ilgili Capsule'ı çekip tipi kontrol etmemiz gerekir.
            std::optional<CerebrumLux::Capsule> capsule = learningModule.getKnowledgeBase().find_capsule_by_id(selectedCapsuleId.toStdString());
            if (capsule) {
                // Not: Capsule struct'ının kendisinde InsightType diye bir alan yok.
                // Eğer bu bilgiyi kapsülün içinde tutmuyorsak, ID'nin önekinden (CodeDevSuggestion_) çıkarım yapabiliriz.
                // Veya Insight sınıfına bu bilgiyi ekleyip KnowledgeCapsuleDisplayData'ya aktarmamız gerekir.
                // Şimdilik ID önekinden yola çıkarak bir simülasyon yapalım.
                if (selectedCapsuleId.startsWith("CodeDevSuggest")) { // CodeDevSuggest_ veya CodeDevSuggest_HighComplexity_ vs.
                    enableButtons = true;
                }
            }
        }
    }
    acceptSuggestionButton->setEnabled(enableButtons);
    rejectSuggestionButton->setEnabled(enableButtons);
}

// YENİ SLOT: Öneriyi Kabul Et butonu tıklandığında
void CerebrumLux::KnowledgeBasePanel::onAcceptSuggestionClicked() {
    if (capsuleListWidget->currentItem()) {
        QString selectedCapsuleId = capsuleListWidget->currentItem()->data(Qt::UserRole).toString();
        LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "KnowledgeBasePanel: Kod Geliştirme Önerisi KABUL EDİLDİ. ID: " << selectedCapsuleId.toStdString());
        // LearningModule'e geri bildirim gönder
        learningModule.processCodeSuggestionFeedback(selectedCapsuleId.toStdString(), true);
        // Opsiyonel: Kabul edilen öneri kapsülünü karantinaya alabilir veya farklı bir şekilde işaretleyebiliriz.
        QMessageBox::information(this, "Öneri Kabul Edildi", "Kod geliştirme önerisi kabul edildi: " + selectedCapsuleId);
        updateKnowledgeBaseContent(); // Listeyi güncelle
    } else {
        QMessageBox::warning(this, "Uyarı", "Lütfen kabul etmek için bir kod geliştirme önerisi seçin.");
    }
}

// YENİ SLOT: Öneriyi Reddet butonu tıklandığında
void CerebrumLux::KnowledgeBasePanel::onRejectSuggestionClicked() {
    if (capsuleListWidget->currentItem()) {
        QString selectedCapsuleId = capsuleListWidget->currentItem()->data(Qt::UserRole).toString();
        LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "KnowledgeBasePanel: Kod Geliştirme Önerisi REDDEDİLDİ. ID: " << selectedCapsuleId.toStdString());
        // LearningModule'e geri bildirim gönder
        learningModule.processCodeSuggestionFeedback(selectedCapsuleId.toStdString(), false);
        // Opsiyonel: Reddedilen öneri kapsülünü karantinaya alabilir veya farklı bir şekilde işaretleyebiliriz.
        QMessageBox::information(this, "Öneri Reddedildi", "Kod geliştirme önerisi reddedildi: " + selectedCapsuleId);
        updateKnowledgeBaseContent(); // Listeyi güncelle
    } else {
        QMessageBox::warning(this, "Uyarı", "Lütfen reddetmek için bir kod geliştirme önerisi seçin.");
    }
}

} // namespace CerebrumLux