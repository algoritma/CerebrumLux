#include "KnowledgeBasePanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QHeaderView>
#include <QRegularExpression>
#include <QLabel>
#include <QSet> // uniqueTopics için
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

    // Yeni bir Splitter oluşturarak ana içeriği (kapsül listesi/detay) ve ilgili kapsülleri ayırabiliriz.
    QSplitter *mainSplitter = new QSplitter(Qt::Horizontal, this);
 
    // 🔍 Arama kutusu
    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchLineEdit = new QLineEdit(this);
    searchLineEdit->setPlaceholderText("Kapsüllerde ara (ID, Konu, Özet)...");
    connect(searchLineEdit, &QLineEdit::textChanged, this, &CerebrumLux::KnowledgeBasePanel::onSearchTextChanged);

    clearSearchButton = new QPushButton("Temizle", this);
    connect(clearSearchButton, &QPushButton::clicked, this, &CerebrumLux::KnowledgeBasePanel::onClearSearchClicked);

    searchLayout->addWidget(searchLineEdit);
    searchLayout->addWidget(clearSearchButton);
    mainLayout->addLayout(searchLayout);

    // 📂 Filtreleme Kontrolleri
    QHBoxLayout *filterLayout = new QHBoxLayout();

    filterLayout->addWidget(new QLabel("Konu:", this));
    topicFilterComboBox = new QComboBox(this);
    topicFilterComboBox->addItem("Tümü");
    connect(topicFilterComboBox, &QComboBox::currentTextChanged, this, &CerebrumLux::KnowledgeBasePanel::onTopicFilterChanged);
    filterLayout->addWidget(topicFilterComboBox);

    // ✅ Özel Filtre: Sadece Code Development gibi
    filterLayout->addWidget(new QLabel("Özel Filtre:", this));
    specialFilterComboBox = new QComboBox(this);
    specialFilterComboBox->addItem("Tümü");
    specialFilterComboBox->addItem("Sadece Code Development");
    connect(specialFilterComboBox, &QComboBox::currentTextChanged, this, &CerebrumLux::KnowledgeBasePanel::onSpecialFilterChanged);
    filterLayout->addWidget(specialFilterComboBox);

    filterLayout->addWidget(new QLabel("Başlangıç Tarihi:", this));
    startDateEdit = new QDateEdit(QDate(2000, 1, 1), this);
    startDateEdit->setCalendarPopup(true);
    connect(startDateEdit, &QDateEdit::dateChanged, this, &CerebrumLux::KnowledgeBasePanel::onStartDateChanged);
    filterLayout->addWidget(startDateEdit);

    filterLayout->addWidget(new QLabel("Bitiş Tarihi:", this));
    endDateEdit = new QDateEdit(QDate::currentDate().addYears(1), this); // Bugün + 1 yıl
    endDateEdit->setCalendarPopup(true);
    connect(endDateEdit, &QDateEdit::dateChanged, this, &CerebrumLux::KnowledgeBasePanel::onEndDateChanged);
    filterLayout->addWidget(endDateEdit);

    filterLayout->addStretch();
    mainLayout->addLayout(filterLayout);

    // 🔗 Splitter: Liste + Detay
    QSplitter *detailSplitter = new QSplitter(Qt::Vertical, this); // Dikey splitter
    capsuleListWidget = new QListWidget(this);
    capsuleListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(capsuleListWidget, &QListWidget::currentItemChanged, this, &CerebrumLux::KnowledgeBasePanel::onSelectedCapsuleChanged);
    detailSplitter->addWidget(capsuleListWidget);

    capsuleDetailDisplay = new QTextEdit(this);
    capsuleDetailDisplay->setReadOnly(true);
    detailSplitter->addWidget(capsuleDetailDisplay);

    mainSplitter->addWidget(detailSplitter); // Ana splitter'ın sol tarafına kapsül listesi ve detay

    // YENİ UI elemanları: İlgili Kapsüller için
    QVBoxLayout *relatedLayout = new QVBoxLayout();
    relatedLayout->addWidget(new QLabel("İlgili Kapsüller:", this));
    relatedCapsuleListWidget = new QListWidget(this);
    relatedLayout->addWidget(relatedCapsuleListWidget);
    
    QWidget *relatedWidget = new QWidget(this);
    relatedWidget->setLayout(relatedLayout);
    mainSplitter->addWidget(relatedWidget); // Ana splitter'ın sağ tarafına ilgili kapsüller

    mainSplitter->setSizes({width() / 2, width() / 2}); // Başlangıçta eşit genişlik
    mainLayout->addWidget(mainSplitter);

    // 👍👎 Geri bildirim butonları
    QHBoxLayout *feedbackLayout = new QHBoxLayout();
    acceptSuggestionButton = new QPushButton("Öneriyi Kabul Et", this);
    rejectSuggestionButton = new QPushButton("Öneriyi Reddet", this);
    feedbackLayout->addWidget(acceptSuggestionButton);
    feedbackLayout->addWidget(rejectSuggestionButton);
    mainLayout->addLayout(feedbackLayout);

    connect(acceptSuggestionButton, &QPushButton::clicked, this, &CerebrumLux::KnowledgeBasePanel::onAcceptSuggestionClicked);
    connect(rejectSuggestionButton, &QPushButton::clicked, this, &CerebrumLux::KnowledgeBasePanel::onRejectSuggestionClicked);
    updateSuggestionFeedbackButtons(""); // Başlangıçta butonları pasif yap

    mainLayout->addStretch(1);
    setLayout(mainLayout);
}
// 
void KnowledgeBasePanel::updateKnowledgeBaseContent() {
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "KnowledgeBasePanel: KnowledgeBase içeriği güncelleniyor.");

    QString selectedCapsuleId;
    if (capsuleListWidget->currentItem()) {
        selectedCapsuleId = capsuleListWidget->currentItem()->data(Qt::UserRole).toString();
        LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "KnowledgeBasePanel: Mevcut secili kapsul ID: " << selectedCapsuleId.toStdString());
    } // else { selectedCapsuleId kalır}

    currentDisplayedCapsules = learningModule.getKnowledgeBase().get_all_capsules(); // Tüm kapsülleri al (Kapsül sayisi logu kaldirildi, cünkü amacina ulasti)

    // Mevcut seçili konuyu kaydet
    QString currentTopicSelection = topicFilterComboBox->currentText();
    LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "KnowledgeBasePanel: Mevcut secili konu: " << currentTopicSelection.toStdString());

    // Konu filtrelerini yeniden doldur
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

    // Önceden seçili konuyu geri yükle, yoksa "Tümü"nü seç
    int index = topicFilterComboBox->findText(currentTopicSelection);
    if (index != -1) {
        topicFilterComboBox->setCurrentIndex(index);
        LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "KnowledgeBasePanel: Onceki konu secimi geri yuklendi: " << currentTopicSelection.toStdString());
    } else {
        topicFilterComboBox->setCurrentText("Tümü");
        LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "KnowledgeBasePanel: Onceki konu secimi listede bulunamadi, 'Tumu' secildi.");
    }

    topicFilterComboBox->blockSignals(false); // Sinyalleri tekrar etkinleştir

    // Mevcut filtrelerle listeyi yeniden doldur
    // ÖNEMLİ: updateKnowledgeBaseContent() içinde çağrılan filterAndDisplayCapsules() metoduna
    // boş current_capsule_embedding parametresi göndermeliyiz, çünkü burada seçili bir kapsül yok.
    // Seçili kapsül değiştiğinde, onSelectedCapsuleChanged() metodunun filterAndDisplayCapsules()'ı çağırması gerekecek.
    // Şimdilik, sadece mevcut filtreleme metodu çağrılıyor ve embedding ile ilgili kısım daha sonra eklenecek.

    filterAndDisplayCapsules(searchLineEdit->text(),
    topicFilterComboBox->currentText(),
    startDateEdit->date(), endDateEdit->date(),
    specialFilterComboBox->currentText()); // Yeni filtre parametresi eklendi

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
            // Seçilen kapsül geri yüklendiğinde detayları ve ilgili kapsülleri de güncelle
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
        relatedCapsuleListWidget->clear(); // YENİ: Seçim yoksa ilgili kapsülleri de temizle
        updateSuggestionFeedbackButtons(""); // ✅ Seçim yoksa butonları pasif yap
        return;
    }

    QString selectedCapsuleId = current->data(Qt::UserRole).toString();
    auto it = displayedCapsuleDetails.find(selectedCapsuleId);
    if (it != displayedCapsuleDetails.end()) {
        displayCapsuleDetails(it->second);
        LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "KnowledgeBasePanel: Kapsül detayları gösterildi. ID: " << selectedCapsuleId.toStdString());
        // YENİ: İlgili kapsülleri de güncelle
        updateRelatedCapsules(selectedCapsuleId.toStdString(), it->second.embedding);
    } else {
        updateSuggestionFeedbackButtons("");
        capsuleDetailDisplay->setText("Detaylar bulunamadı.");
        relatedCapsuleListWidget->clear();
        LOG_DEFAULT(CerebrumLux::LogLevel::WARNING, "KnowledgeBasePanel: Seçilen kapsül ID'si dahili listeye uymuyor: " << selectedCapsuleId.toStdString());
    }
}

void KnowledgeBasePanel::onSearchTextChanged(const QString& text) {
    filterAndDisplayCapsules(text,
                             topicFilterComboBox->currentText(),
                             startDateEdit->date(),
                             endDateEdit->date(),
                             specialFilterComboBox->currentText()); // Yeni filtre parametresi eklendi
}

void KnowledgeBasePanel::onClearSearchClicked() {
    searchLineEdit->clear();
    filterAndDisplayCapsules(QString(),
                             topicFilterComboBox->currentText(),
                             startDateEdit->date(),
                             endDateEdit->date(),
                             specialFilterComboBox->currentText()); // Yeni filtre parametresi eklendi
}

// YENİ SLOTLARIN İMPLEMENTASYONLARI
void KnowledgeBasePanel::onTopicFilterChanged(const QString& topic) {
    filterAndDisplayCapsules(searchLineEdit->text(),
                             topic,
                             startDateEdit->date(),
                             endDateEdit->date(),
                             specialFilterComboBox->currentText()); // Yeni filtre parametresi eklendi
}

// ✅ YENİ SLOT: CodeDevelopment filtre kontrolü için
void KnowledgeBasePanel::onSpecialFilterChanged(const QString& filter) {
    filterAndDisplayCapsules(searchLineEdit->text(),
                             topicFilterComboBox->currentText(),
                             startDateEdit->date(),
                             endDateEdit->date(),
                             filter); // Yeni filtre parametresi kullanıldı
}

void KnowledgeBasePanel::onStartDateChanged(const QDate& date) {
    filterAndDisplayCapsules(searchLineEdit->text(),
                             topicFilterComboBox->currentText(),
                             date,
                             endDateEdit->date(),
                             specialFilterComboBox->currentText()); // Yeni filtre parametresi eklendi
}

void KnowledgeBasePanel::onEndDateChanged(const QDate& date) {
    filterAndDisplayCapsules(searchLineEdit->text(),
                             topicFilterComboBox->currentText(),
                             startDateEdit->date(),
                             date,
                             specialFilterComboBox->currentText()); // Yeni filtre parametresi eklendi
}

void KnowledgeBasePanel::filterAndDisplayCapsules(const QString& filterText,
                                                  const QString& topicFilter,
                                                  const QDate& startDate,
                                                  const QDate& endDate,
                                                  const QString& specialFilter) { // ✅ Yeni parametre
    capsuleListWidget->clear();
    displayedCapsuleDetails.clear();

    LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "KnowledgeBasePanel: Kapsül filtreleme baslatildi. Toplam kapsül: " << currentDisplayedCapsules.size() << ", Özel Filtre: '" << specialFilter.toStdString() << "'"); // Log seviyesi TRACE'e düşürüldü

    for (const auto& capsule : currentDisplayedCapsules) {        
        QString capsuleId = QString::fromStdString(capsule.id);
        QString topic = QString::fromStdString(capsule.topic);
        QString capsuleSource = QString::fromStdString(capsule.source);
        QString capsuleSummary = QString::fromStdString(capsule.plain_text_summary);
        // ✅ DÜZELTME: Nanosaniyeden saniyeye dönüştürme yapıldı
        if (capsule.timestamp_utc.time_since_epoch().count() == 0) { // Zaman damgası 0 ise varsayılan tarih ayarla (hata önleme)
             /* LOG_DEFAULT(CerebrumLux::LogLevel::WARNING, "KnowledgeBasePanel: Kapsül ID'si " << capsuleId.toStdString() << " icin gecersiz zaman damgasi (0) tespit edildi. Varsayilan tarih kullanilacak."); */ // Aşırı loglamayı engellemek için yorum satırı yapıldı
        }
        // Unix Epoch'tan itibaren nanosaniyeleri saniyeye çeviriyoruz.
        // std::chrono::system_clock::time_point nanosecond bazlı olabilir, QDateTime::fromSecsSinceEpoch saniye bekler.
        auto epoch_nanos = capsule.timestamp_utc.time_since_epoch();
        auto epoch_secs = std::chrono::duration_cast<std::chrono::seconds>(epoch_nanos);
        QDateTime dt = QDateTime::fromSecsSinceEpoch(epoch_secs.count()); 
        QDate capsuleDate = dt.date();


        // ✅ Özel filtre: yalnızca CodeDevelopment kapsülleri
        if (specialFilter == "Sadece Code Development" && topic != "CodeDevelopment") {
            /* LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "KnowledgeBasePanel: Kapsül özel filtreden geçmedi (Code Development değil). ID: " << capsuleId.toStdString()); */ // Aşırı loglamayı engellemek için yorum satırı yapıldı
            continue; // CodeDevelopment değilse atla
        }
        // ✅ TEŞHİS: Filtreleme koşullarını ayrı ayrı değerlendir
        bool matchesSearch = (filterText.isEmpty() ||
                              capsuleId.contains(filterText, Qt::CaseInsensitive) ||
                              topic.contains(filterText, Qt::CaseInsensitive) ||
                              capsuleSource.contains(filterText, Qt::CaseInsensitive) ||
                              capsuleSummary.contains(filterText, Qt::CaseInsensitive) ||
                              QString::fromStdString(capsule.code_file_path).contains(filterText, Qt::CaseInsensitive)); // code_file_path de aramaya dahil edildi

        bool matchesTopic = (topicFilter == "Tümü" || topic.compare(topicFilter, Qt::CaseInsensitive) == 0);
        bool matchesStartDate = (!startDate.isValid() || capsuleDate >= startDate);
        bool matchesEndDate = (!endDate.isValid() || capsuleDate <= endDate);

        bool matches = matchesSearch && matchesTopic && matchesStartDate && matchesEndDate;
        if (!matches) {
            /* LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "KnowledgeBasePanel: Kapsül diğer filtrelerden geçmedi. ID: " << capsuleId.toStdString() << " (Arama: " << (matchesSearch ? "Gecti" : "Kaldi") << ", Konu: " << (matchesTopic ? "Gecti" : "Kaldi") << ", Baslangic Tarihi: " << (matchesStartDate ? "Gecti" : "Kaldi") << ", Bitis Tarihi: " << (matchesEndDate ? "Gecti" : "Kaldi") << ", Kapsül Tarihi: " << capsuleDate.toString("dd.MM.yyyy").toStdString() << ", Filtre Baslangic: " << startDate.toString("dd.MM.yyyy").toStdString() << ", Filtre Bitis: " << endDate.toString("dd.MM.yyyy").toStdString() << ")");            */
             continue;
        }
        //LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "KnowledgeBasePanel: Kapsül tüm filtrelerden gecti ve listeye ekleniyor. ID: " << capsuleId.toStdString()); // Daha genel bir log

        QString itemText = QString("ID: %1 | Konu: %2 | Kaynak: %3 | Tarih: %4 | Dosya: %5")
                             .arg(capsuleId)
                             .arg(topic)
                             .arg(capsuleSource)
                             .arg(dt.toString("dd.MM.yyyy hh:mm"))
                             .arg(QString::fromStdString(capsule.code_file_path)); // ✅ EKLENDİ: code_file_path itemText'e eklendi
        
        QListWidgetItem *item = new QListWidgetItem(itemText);
        item->setData(Qt::UserRole, capsuleId);
        capsuleListWidget->addItem(item);

        KnowledgeCapsuleDisplayData data; // Yeni data objesi olusturuldu
        data.id = capsuleId;
        data.topic = topic;
        data.source = capsuleSource;
        data.summary = capsuleSummary;
        data.fullContent = QString::fromStdString(capsule.content);
        data.cryptofigBlob = QString::fromStdString(capsule.cryptofig_blob_base64);
        data.confidence = capsule.confidence;
        data.code_file_path = QString::fromStdString(capsule.code_file_path);
        data.embedding = capsule.embedding; // YENİ: Embedding'i de kaydet
        displayedCapsuleDetails[capsuleId] = data; // Data objesi mapa eklendi

        LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "KnowledgeBasePanel: Kapsül QListWidget'a eklendi. ID: " << capsuleId.toStdString()); // Log seviyesi Trace'e düşürüldü
    } // for döngüsü burada bitiyor
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "KnowledgeBasePanel: filterAndDisplayCapsules tamamlandi. Toplam listelenen kapsül: " << capsuleListWidget->count());
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
    // Daha önce vardı, şimdi düzgün atama ile beraber çalışmalı
    // Eski satır: details += "<b>Dosya Yolu:</b> " + data.codeFilePath + "<br>";
    if (!data.code_file_path.isEmpty()) { 
        details += "<b>Dosya Yolu:</b> " + data.code_file_path + "<br>";
    }
    // YENİ EKLENDİ: Embedding vektörünü göster
    details += "<b>Embedding (İlk 10 Eleman):</b> [";
    for (int i = 0; i < std::min((int)data.embedding.size(), 10); ++i) {
        details += QString::number(data.embedding[i], 'f', 4) + (i == std::min((int)data.embedding.size(), 10) - 1 ? "" : ", ");
    }
    details += "]...<br>";
    details += "<br><b>Tam İçerik:</b><br><pre>" + data.fullContent + "</pre>"; // Moved out of if block

    capsuleDetailDisplay->setHtml(details);
    updateSuggestionFeedbackButtons(data.id);
}

void KnowledgeBasePanel::updateRelatedCapsules(const std::string& current_capsule_id, const std::vector<float>& current_capsule_embedding) {
    relatedCapsuleListWidget->clear();
    currentRelatedCapsules.clear();

    if (current_capsule_embedding.empty()) {
        LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "KnowledgeBasePanel: current_capsule_embedding boş, ilgili kapsüller listelenemedi. ID: " << current_capsule_id);
        return;
    }

    // Seçilen kapsülün embedding'ini kullanarak ilgili kapsülleri ara
    std::vector<CerebrumLux::Capsule> related_caps = learningModule.getKnowledgeBase().semantic_search(current_capsule_embedding, 5); // En yakın 5 kapsülü ara

    LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "KnowledgeBasePanel: ID: " << current_capsule_id << " icin semantik arama yapildi. Bulunan ilgili kapsül sayisi: " << related_caps.size());

    for (const auto& rel_capsule : related_caps) {
        if (rel_capsule.id == current_capsule_id) { // Kendisini listeden çıkar
            continue;
        }
        QString itemText = QString("ID: %1 | Konu: %2 | Özet: %3")
                            .arg(QString::fromStdString(rel_capsule.id))
                            .arg(QString::fromStdString(rel_capsule.topic))
                            .arg(QString::fromStdString(rel_capsule.plain_text_summary).left(50) + "...");
        
        QListWidgetItem *item = new QListWidgetItem(itemText);
        item->setData(Qt::UserRole, QString::fromStdString(rel_capsule.id));
        relatedCapsuleListWidget->addItem(item);
        currentRelatedCapsules.push_back(rel_capsule); // İlgili kapsülleri sakla
        LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "KnowledgeBasePanel: İlgili kapsül listesine eklendi. ID: " << rel_capsule.id);
    }
    if (related_caps.empty()) {
        relatedCapsuleListWidget->addItem("İlgili kapsül bulunamadı.");
    }
}

void KnowledgeBasePanel::updateSuggestionFeedbackButtons(const QString& selectedId) {
    bool enable = false;
    if (!selectedId.isEmpty()) {
        auto it = displayedCapsuleDetails.find(selectedId);
        if (it != displayedCapsuleDetails.end()) {
            if (it->second.topic == "CodeDevelopment") // Sadece CodeDevelopment kapsülleri için aktif et
                enable = true;
        }
    }
    acceptSuggestionButton->setEnabled(enable);
    rejectSuggestionButton->setEnabled(enable);
}

void KnowledgeBasePanel::onAcceptSuggestionClicked() {
    if (!capsuleListWidget->currentItem()) return;
    QString id = capsuleListWidget->currentItem()->data(Qt::UserRole).toString();
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "KnowledgeBasePanel: Kod Geliştirme Önerisi KABUL EDİLDİ. ID: " << id.toStdString());
    // LearningModule'e geri bildirim gönder
    learningModule.processCodeSuggestionFeedback(id.toStdString(), true);
    QMessageBox::information(this, "Öneri Kabul", "Kod geliştirme önerisi kabul edildi: " + id);
    updateKnowledgeBaseContent();
}

void KnowledgeBasePanel::onRejectSuggestionClicked() {
    if (!capsuleListWidget->currentItem()) return;
    QString id = capsuleListWidget->currentItem()->data(Qt::UserRole).toString();
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "KnowledgeBasePanel: Kod Geliştirme Önerisi REDDEDİLDİ. ID: " << id.toStdString());
    // LearningModule'e geri bildirim gönder
    learningModule.processCodeSuggestionFeedback(id.toStdString(), false);
    QMessageBox::information(this, "Öneri Reddedildi", "Kod geliştirme önerisi reddedildi: " + id);
    updateKnowledgeBaseContent();
}

} // namespace CerebrumLux
