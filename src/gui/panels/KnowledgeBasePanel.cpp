#include "KnowledgeBasePanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QRegularExpression>
#include <QLabel>
#include <QSet>
#include <QThread> // QThread için
#include <QDateTime> // QDateTime için
#include <algorithm> // std::min için
#include "../../core/logger.h"
#include "../../core/utils.h"

namespace CerebrumLux {

KnowledgeBasePanel::KnowledgeBasePanel(LearningModule& learningModuleRef, QWidget *parent)
    : QWidget(parent),
      learningModule(learningModuleRef),
      mainSplitter(nullptr), // Üye başlatma listesinde başlatıldı.
      capsuleListWidget(nullptr), capsuleDetailDisplay(nullptr),
      searchLineEdit(nullptr), clearSearchButton(nullptr),
      topicFilterComboBox(nullptr), specialFilterComboBox(nullptr),
      startDateEdit(nullptr), endDateEdit(nullptr),
      relatedCapsuleListWidget(nullptr),
      acceptSuggestionButton(nullptr), rejectSuggestionButton(nullptr)
{
    setupUi();

    knowledgeBaseWorker = new KnowledgeBaseWorker(learningModule.getKnowledgeBase(), nullptr);
    knowledgeBaseWorker->moveToThread(&workerThread);
    
    connect(&workerThread, &QThread::finished, knowledgeBaseWorker, &QObject::deleteLater);
    connect(knowledgeBaseWorker, &KnowledgeBaseWorker::allCapsulesFetched, this, &KnowledgeBasePanel::handleAllCapsulesFetched);
    connect(knowledgeBaseWorker, &KnowledgeBaseWorker::relatedCapsulesFetched, this, &KnowledgeBasePanel::handleRelatedCapsulesFetched);
    connect(knowledgeBaseWorker, &KnowledgeBaseWorker::workerError, this, &KnowledgeBasePanel::handleWorkerError);

    // Panelden worker'a gönderilecek sinyali bağla
    connect(this, &KnowledgeBasePanel::requestFetchAllCapsulesAndDetails,
            knowledgeBaseWorker, &KnowledgeBaseWorker::fetchAllCapsulesAndDetails,
            Qt::QueuedConnection);
    connect(this, &KnowledgeBasePanel::requestFetchRelatedCapsules,
            knowledgeBaseWorker, &KnowledgeBaseWorker::fetchRelatedCapsules,
            Qt::QueuedConnection);

    workerThread.start(); // İşçi iş parçacığını başlat

    LOG_DEFAULT(LogLevel::INFO, "KnowledgeBasePanel: Initialized.");
    // İlk içerik güncellemesini applyFiltersAndFetchData() ile yapıyoruz.
    // Bu, filtreleme kontrolleri tamamen yüklendikten sonra worker'a ilk isteği gönderecektir.
    applyFiltersAndFetchData();
}

KnowledgeBasePanel::~KnowledgeBasePanel() {
    workerThread.quit(); // İş parçacığını durdurma sinyali gönder
    workerThread.wait(1000); // 1 saniye bekle
    if (workerThread.isRunning()) {
        workerThread.terminate(); // Hala çalışıyorsa sonlandır
        workerThread.wait(500);
    }
    delete knowledgeBaseWorker; // Worker nesnesini sil

    LOG_DEFAULT(LogLevel::INFO, "KnowledgeBasePanel: Destructor called.");
}


void KnowledgeBasePanel::setupUi() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    mainLayout->addWidget(new QLabel("KnowledgeBase İçeriği:", this));

    mainSplitter = new QSplitter(Qt::Horizontal, this); // Sınıf üyesi olduğu için tekrar tanımlama yok.
 
    // Arama kutusu
    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchLineEdit = new QLineEdit(this);
    searchLineEdit->setPlaceholderText("Kapsüllerde ara (ID, Konu, Özet)...");
    connect(searchLineEdit, &QLineEdit::textChanged, this, &CerebrumLux::KnowledgeBasePanel::onSearchTextChanged);

    clearSearchButton = new QPushButton("Temizle", this);
    connect(clearSearchButton, &QPushButton::clicked, this, &CerebrumLux::KnowledgeBasePanel::onClearSearchClicked);

    searchLayout->addWidget(searchLineEdit);
    searchLayout->addWidget(clearSearchButton);
    mainLayout->addLayout(searchLayout);

    // Filtreleme Kontrolleri
    QHBoxLayout *filterLayout = new QHBoxLayout();

    filterLayout->addWidget(new QLabel("Konu:", this));
    topicFilterComboBox = new QComboBox(this);
    topicFilterComboBox->blockSignals(true); // Sinyalleri geçici olarak blokla
    topicFilterComboBox->addItem("Tümü");
    topicFilterComboBox->blockSignals(false); // Sinyalleri tekrar etkinleştir
    connect(topicFilterComboBox, &QComboBox::currentTextChanged, this, &CerebrumLux::KnowledgeBasePanel::onTopicFilterChanged);
    filterLayout->addWidget(topicFilterComboBox);

    filterLayout->addWidget(new QLabel("Özel Filtre:", this));
    specialFilterComboBox = new QComboBox(this);
    specialFilterComboBox->blockSignals(true); // Sinyalleri geçici olarak blokla
    specialFilterComboBox->addItem("Tümü");
    specialFilterComboBox->addItem("Sadece Code Development");
    specialFilterComboBox->blockSignals(false); // Sinyalleri tekrar etkinleştir
    connect(specialFilterComboBox, &QComboBox::currentTextChanged, this, &CerebrumLux::KnowledgeBasePanel::onSpecialFilterChanged);
    filterLayout->addWidget(specialFilterComboBox);

    filterLayout->addWidget(new QLabel("Başlangıç Tarihi:", this));
    startDateEdit = new QDateEdit(QDate(2000, 1, 1), this);
    startDateEdit->setCalendarPopup(true);
    connect(startDateEdit, &QDateEdit::dateChanged, this, &CerebrumLux::KnowledgeBasePanel::onStartDateChanged);
    filterLayout->addWidget(startDateEdit);

    filterLayout->addWidget(new QLabel("Bitiş Tarihi:", this));
    endDateEdit = new QDateEdit(QDate::currentDate().addYears(1), this);
    endDateEdit->setCalendarPopup(true);
    connect(endDateEdit, &QDateEdit::dateChanged, this, &CerebrumLux::KnowledgeBasePanel::onEndDateChanged);
    filterLayout->addWidget(endDateEdit);

    filterLayout->addStretch();
    mainLayout->addLayout(filterLayout);

    // Detay Splitter (Kapsül listesi ve detaylar)
    QSplitter *detailSplitter = new QSplitter(Qt::Vertical, this);
    capsuleListWidget = new QListWidget(this);
    capsuleListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(capsuleListWidget, &QListWidget::currentItemChanged, this, &CerebrumLux::KnowledgeBasePanel::onSelectedCapsuleChanged);
    detailSplitter->addWidget(capsuleListWidget);

    capsuleDetailDisplay = new QTextEdit(this);
    capsuleDetailDisplay->setReadOnly(true);
    detailSplitter->addWidget(capsuleDetailDisplay);

    mainSplitter->addWidget(detailSplitter); // Ana splitter'ın sol tarafına kapsül listesi ve detay.

    // İlgili Kapsüller için UI elemanları
    QVBoxLayout *relatedLayout = new QVBoxLayout();
    relatedLayout->addWidget(new QLabel("İlgili Kapsüller:", this));
    relatedCapsuleListWidget = new QListWidget(this);
    relatedLayout->addWidget(relatedCapsuleListWidget); 
    connect(relatedCapsuleListWidget, &QListWidget::itemClicked, this, &CerebrumLux::KnowledgeBasePanel::onRelatedCapsuleClicked);
    
    QWidget *relatedWidget = new QWidget(this);
    relatedWidget->setLayout(relatedLayout);
    mainSplitter->addWidget(relatedWidget); // Ana splitter'ın sağ tarafına ilgili kapsüller

    mainSplitter->setSizes({width() / 2, width() / 2});
    mainLayout->addWidget(mainSplitter);

    // Geri bildirim butonları
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

void KnowledgeBasePanel::updateKnowledgeBaseContent() {
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "KnowledgeBasePanel: KnowledgeBase içeriği güncelleniyor.");
    applyFiltersAndFetchData(); // Worker'a veri çekme isteği gönder.
}

void KnowledgeBasePanel::handleAllCapsulesFetched(const std::vector<Capsule>& all_capsules,
                                                const std::map<QString, KnowledgeCapsuleDisplayData>& displayed_details,
                                                const QSet<QString>& unique_topics,
                                                const QString& restoreSelectionId) {
    LOG_DEFAULT(LogLevel::DEBUG, "KnowledgeBasePanel: Worker'dan kapsül verileri alindi. Toplam kapsül: " << all_capsules.size());

    currentDisplayedCapsules = all_capsules;
    displayedCapsuleDetails = displayed_details;

    // Konu filtrelerini yeniden doldur
    QString currentTopicSelection = topicFilterComboBox->currentText();
    topicFilterComboBox->blockSignals(true);
    topicFilterComboBox->clear();
    topicFilterComboBox->addItem("Tümü");
    for (const QString& topic : unique_topics) { topicFilterComboBox->addItem(topic); }
    int index = topicFilterComboBox->findText(currentTopicSelection);
    if (index != -1) { topicFilterComboBox->setCurrentIndex(index); } else { topicFilterComboBox->setCurrentText("Tümü"); }
    topicFilterComboBox->blockSignals(false);

    // Listeyi doldur
    capsuleListWidget->clear();
    for (const auto& pair : displayed_details) {
        QString capsuleId = pair.first;
        const KnowledgeCapsuleDisplayData& data = pair.second;
        
        QString itemText = QString("ID: %1 | Konu: %2 | Kaynak: %3 | Tarih: %4 | Dosya: %5")
                             .arg(data.id)
                             .arg(data.topic)
                             .arg(data.source)
                             .arg(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(data.timestamp_utc.time_since_epoch()).count()).toString("dd.MM.yyyy hh:mm"))
                             .arg(data.code_file_path);

        QListWidgetItem *item = new QListWidgetItem(itemText);
        item->setData(Qt::UserRole, capsuleId);
        capsuleListWidget->addItem(item);
    }

    // Seçimi geri yükle
    if (!restoreSelectionId.isEmpty()) {
        QListWidgetItem *itemToSelect = nullptr;
        for (int i = 0; i < capsuleListWidget->count(); ++i) {
            QListWidgetItem *item = capsuleListWidget->item(i);
            if (item->data(Qt::UserRole).toString() == restoreSelectionId) {
                itemToSelect = item;
                break;
            }
        }
        if (itemToSelect) { capsuleListWidget->setCurrentItem(itemToSelect); } else { capsuleDetailDisplay->clear(); }
    } else { capsuleDetailDisplay->clear(); }
    
    LOG_DEFAULT(LogLevel::DEBUG, "KnowledgeBasePanel: KnowledgeBase içeriği GUI'de güncellendi. Toplam listelenen: " << capsuleListWidget->count());
}

void KnowledgeBasePanel::handleRelatedCapsulesFetched(const std::vector<Capsule>& related_capsules, const std::string& for_capsule_id) {
    LOG_DEFAULT(LogLevel::DEBUG, "KnowledgeBasePanel: Worker'dan ilgili kapsül verileri alindi. ID: " << for_capsule_id << ", Sayi: " << related_capsules.size());

    relatedCapsuleListWidget->clear();
    currentRelatedCapsules = related_capsules;

    if (related_capsules.empty()) {
        relatedCapsuleListWidget->addItem("İlgili kapsül bulunamadı.");
        return;
    }
    for (const auto& rel_capsule : related_capsules) {
        QString itemText = QString("ID: %1 | Konu: %2 | Özet: %3")
                            .arg(QString::fromStdString(rel_capsule.id))
                            .arg(QString::fromStdString(rel_capsule.topic))
                            .arg(QString::fromStdString(rel_capsule.plain_text_summary).left(50) + "...");
        
        QListWidgetItem *item = new QListWidgetItem(itemText);
        item->setData(Qt::UserRole, QString::fromStdString(rel_capsule.id));
        relatedCapsuleListWidget->addItem(item);
    }
}

void KnowledgeBasePanel::handleWorkerError(const QString& error_message) {
    LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "KnowledgeBasePanel: Worker hatası: " << error_message.toStdString());
    QMessageBox::critical(this, "Hata", "Veri çekme sırasında bir hata oluştu: " + error_message);
}

void KnowledgeBasePanel::onSelectedCapsuleChanged(QListWidgetItem* current, QListWidgetItem* previous) {
    Q_UNUSED(previous);
    if (!current) {
        capsuleDetailDisplay->clear();
        relatedCapsuleListWidget->clear();
        updateSuggestionFeedbackButtons("");
        return;
    }

    QString selectedCapsuleId = current->data(Qt::UserRole).toString();
    auto it = displayedCapsuleDetails.find(selectedCapsuleId);
    if (it != displayedCapsuleDetails.end()) {
        displayCapsuleDetails(it->second);
        LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "KnowledgeBasePanel: Kapsül detayları gösterildi. ID: " << selectedCapsuleId.toStdString());
        Q_EMIT requestFetchRelatedCapsules(selectedCapsuleId.toStdString(), it->second.embedding);
    } else {
        updateSuggestionFeedbackButtons("");
        capsuleDetailDisplay->setText("Detaylar bulunamadı.");
        relatedCapsuleListWidget->clear();
        LOG_DEFAULT(CerebrumLux::LogLevel::WARNING, "KnowledgeBasePanel: Seçilen kapsül ID'si dahili listeye uymuyor: " << selectedCapsuleId.toStdString());
    }
}

void KnowledgeBasePanel::onSearchTextChanged(const QString& text) { applyFiltersAndFetchData(); }
void KnowledgeBasePanel::onClearSearchClicked() { searchLineEdit->clear(); applyFiltersAndFetchData(); }
void KnowledgeBasePanel::onTopicFilterChanged(const QString& topic) { applyFiltersAndFetchData(); }
void KnowledgeBasePanel::onSpecialFilterChanged(const QString& filter) { applyFiltersAndFetchData(); }
void KnowledgeBasePanel::onRelatedCapsuleClicked(QListWidgetItem* item) {
    if (!item) return;
    QString clickedCapsuleId = item->data(Qt::UserRole).toString();
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "KnowledgeBasePanel: İlgili kapsül tıklandı. ID: " << clickedCapsuleId.toStdString());

    QListWidgetItem *itemToSelect = nullptr;
    for (int i = 0; i < capsuleListWidget->count(); ++i) {
        QListWidgetItem *mainListItem = capsuleListWidget->item(i);
        if (mainListItem->data(Qt::UserRole).toString() == clickedCapsuleId) {
            itemToSelect = mainListItem;
            break;
        }
    }

    if (itemToSelect) {
        capsuleListWidget->setCurrentItem(itemToSelect);
        LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "KnowledgeBasePanel: Ana listede ilgili kapsül seçildi. ID: " << clickedCapsuleId.toStdString());
    } else {
        LOG_DEFAULT(CerebrumLux::LogLevel::WARNING, "KnowledgeBasePanel: Tıklanan ilgili kapsül ana listede bulunamadı. ID: " << clickedCapsuleId.toStdString());
    }
}

void KnowledgeBasePanel::onStartDateChanged(const QDate& date) { applyFiltersAndFetchData(); }
void KnowledgeBasePanel::onEndDateChanged(const QDate& date) { applyFiltersAndFetchData(); }

void KnowledgeBasePanel::applyFiltersAndFetchData() {
    QString selectedCapsuleId;
    if (capsuleListWidget->currentItem()) { selectedCapsuleId = capsuleListWidget->currentItem()->data(Qt::UserRole).toString(); }

    emit requestFetchAllCapsulesAndDetails(searchLineEdit->text(),
                                          topicFilterComboBox->currentText(),
                                          startDateEdit->date(),
                                          endDateEdit->date(),
                                          specialFilterComboBox->currentText(),
                                          selectedCapsuleId);
}

void KnowledgeBasePanel::filterAndDisplayCapsules(const QString& filterText,
                                                  const QString& topicFilter,
                                                  const QDate& startDate,
                                                  const QDate& endDate,
                                                  const QString& specialFilter) {
    applyFiltersAndFetchData();
}

void KnowledgeBasePanel::displayCapsuleDetails(const KnowledgeCapsuleDisplayData& data) {
    QString details;
    details += "<h3>Kapsül Detayları</h3>";
    details += "<b>ID:</b> " + data.id + "<br>";
    details += "<b>Konu:</b> " + data.topic + "<br>";
    details += "<b>Kaynak:</b> " + data.source + "<br>";
    details += "<b>Tarih:</b> " + QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(data.timestamp_utc.time_since_epoch()).count()).toString("dd.MM.yyyy hh:mm") + "<br>";
    details += "<b>Güven Seviyesi:</b> " + QString::number(data.confidence, 'f', 2) + "<br>";
    details += "<b>Özet:</b> " + data.summary + "<br>";
    details += "<b>Cryptofig (Base64):</b> " + data.cryptofigBlob.left(100) + "...<br>";
    if (!data.code_file_path.isEmpty()) { 
        details += "<b>Dosya Yolu:</b> " + data.code_file_path + "<br>";
    }
    details += "<b>Embedding (İlk 10 Eleman):</b> [";
    for (int i = 0; i < std::min((int)data.embedding.size(), 10); ++i) {
        details += QString::number(data.embedding[i], 'f', 4) + (i == std::min((int)data.embedding.size(), 10) - 1 ? "" : ", ");
    }
    details += "]...<br>";
    details += "<br><b>Tam İçerik:</b><br><pre>" + data.fullContent + "</pre>";

    capsuleDetailDisplay->setHtml(details);
    updateSuggestionFeedbackButtons(data.id);
}

void KnowledgeBasePanel::updateRelatedCapsules(const std::string& current_capsule_id, const std::vector<float>& current_capsule_embedding) {
    // Bu metod artık worker'dan gelen verilerle dolduruluyor.
    // Direct semantic_search call kaldırıldı.
    // fetchRelatedCapsules sinyali onSelectedCapsuleChanged içinde emit ediliyor.
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
    learningModule.processCodeSuggestionFeedback(id.toStdString(), true);
    QMessageBox::information(this, "Öneri Kabul", "Kod geliştirme önerisi kabul edildi: " + id);
    updateKnowledgeBaseContent(); // Günellemeyi tetikle
}

void KnowledgeBasePanel::onRejectSuggestionClicked() {
    if (!capsuleListWidget->currentItem()) return;
    QString id = capsuleListWidget->currentItem()->data(Qt::UserRole).toString();
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "KnowledgeBasePanel: Kod Geliştirme Önerisi REDDEDİLDİ. ID: " << id.toStdString());
    learningModule.processCodeSuggestionFeedback(id.toStdString(), false);
    QMessageBox::information(this, "Öneri Reddedildi", "Kod geliştirme önerisi reddedildi: " + id);
    updateKnowledgeBaseContent();
}

} // namespace CerebrumLux