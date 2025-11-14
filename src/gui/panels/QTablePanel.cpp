#include "QTablePanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMap> // QMap<QString, float> için
#include <QMessageBox> // YENİ EKLENDİ: QMessageBox için
#include <algorithm> // std::min için
#include <utility> // std::pair, std::make_pair için
#include <map> // std::map iteratörleri için

namespace CerebrumLux {

QTablePanel::QTablePanel(LearningModule& learningModuleRef, QWidget *parent)
    : QWidget(parent),
      learningModule(learningModuleRef),
      splitter(nullptr), stateListWidget(nullptr), qValueDetailDisplay(nullptr),
      searchLineEdit(nullptr), clearSearchButton(nullptr)
{
    setupUi();

    qTableWorker = new QTableWorker(learningModule, nullptr); // Worker'ın parent'ı yok.
    qTableWorker->moveToThread(&workerThread);
    connect(&workerThread, &QThread::finished, qTableWorker, &QObject::deleteLater);
    connect(qTableWorker, &QTableWorker::qTableContentFetched, this, &QTablePanel::handleQTableContentFetched);
    connect(qTableWorker, &QTableWorker::workerError, this, &QTablePanel::handleWorkerError);

    connect(this, &QTablePanel::requestFetchQTableContent,
            qTableWorker, &QTableWorker::fetchQTableContent,
            Qt::QueuedConnection);

    workerThread.start();
    LOG_DEFAULT(LogLevel::INFO, "QTablePanel: Initialized.");
    applyFiltersAndFetchData();
}

QTablePanel::~QTablePanel() {
    workerThread.quit();
    workerThread.wait(1000);
    if (workerThread.isRunning()) {
        workerThread.terminate();
        workerThread.wait(500);
    }
    delete qTableWorker;
    LOG_DEFAULT(LogLevel::INFO, "QTablePanel: Destructor called.");
}

void QTablePanel::setupUi() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    mainLayout->addWidget(new QLabel("Sparse Q-Table İçeriği:", this));

    // Arama kutusu
    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchLineEdit = new QLineEdit(this);
    searchLineEdit->setPlaceholderText("Durum anahtarlarında ara...");
    connect(searchLineEdit, &QLineEdit::textChanged, this, &CerebrumLux::QTablePanel::onSearchTextChanged);

    clearSearchButton = new QPushButton("Temizle", this);
    connect(clearSearchButton, &QPushButton::clicked, this, &CerebrumLux::QTablePanel::onClearSearchClicked);

    searchLayout->addWidget(searchLineEdit);
    searchLayout->addWidget(clearSearchButton);
    mainLayout->addLayout(searchLayout);

    // Splitter: Liste + Detay
    splitter = new QSplitter(Qt::Vertical, this);
    stateListWidget = new QListWidget(this);
    stateListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(stateListWidget, &QListWidget::currentItemChanged, this, &CerebrumLux::QTablePanel::onSelectedStateChanged);
    splitter->addWidget(stateListWidget);

    qValueDetailDisplay = new QTextEdit(this);
    qValueDetailDisplay->setReadOnly(true);
    splitter->addWidget(qValueDetailDisplay);

    mainLayout->addWidget(splitter);
    mainLayout->addStretch(1);
    setLayout(mainLayout);
}

void QTablePanel::updateQTableContent() {
    LOG_DEFAULT(LogLevel::DEBUG, "QTablePanel: Q-Table içeriği güncelleniyor.");
    applyFiltersAndFetchData();
}

void QTablePanel::handleQTableContentFetched(const std::vector<CerebrumLux::SwarmVectorDB::EmbeddingStateKey>& all_q_state_keys,
                                             const std::map<QString, QTableDisplayData>& displayed_q_table_details,
                                             const QString& restoreSelectionStateKey) {
    LOG_DEFAULT(LogLevel::DEBUG, "QTablePanel: Worker'dan Q-Table verileri alindi. Toplam durum: " << all_q_state_keys.size());

    currentQStateKeys = all_q_state_keys;
    displayedQTableDetails = displayed_q_table_details;

    //stateListWidget->clear();
    // DÜZELTİLDİ: DEBUG AMAÇLI HER ZAMAN TEMİZLEME.
    // Liste boş görünüyorsa, öğelerin görüntülenip görüntülenmediğini test etmek için her seferinde temizliyoruz.
    // Bu kodu ileride tekrar akıllı temizleme mantığına geri almanız gerekebilir.
        stateListWidget->clear();

    for (const auto& pair : displayed_q_table_details) {
        QString stateKey = pair.first;
        const QTableDisplayData& data = pair.second;

        QString itemText = QString("Durum Anahtarı: %1 | Eylem Sayısı: %2")
                            .arg(stateKey.left(50) + "...")
                            .arg(data.actionQValues.size());
 
        // YENİ EKLENDİ: itemText içeriğini logla
        LOG_DEFAULT(LogLevel::DEBUG, "QTablePanel: Eklenen liste öğesi metni: '" << itemText.toStdString() << "'");
        LOG_DEFAULT(LogLevel::DEBUG, "QTablePanel: stateKey'in ilk 50 karakteri: '" << stateKey.left(50).toStdString() << "'");
        LOG_DEFAULT(LogLevel::DEBUG, "QTablePanel: data.actionQValues.size(): " << data.actionQValues.size());

        // DÜZELTİLDİ: Qt::MatchUserRole hatası giderildi.
        // Öğenin user role verisini kontrol etmek için manuel bir döngüye ihtiyaç var.
            QListWidgetItem *item = new QListWidgetItem(itemText);
            item->setData(Qt::UserRole, stateKey);
            stateListWidget->addItem(item);
    }

    // Seçimi geri yükle
    if (!restoreSelectionStateKey.isEmpty()) {
        QListWidgetItem *itemToSelect = nullptr;
        for (int i = 0; i < stateListWidget->count(); ++i) {
            QListWidgetItem *item = stateListWidget->item(i);
            if (item->data(Qt::UserRole).toString() == restoreSelectionStateKey) {
                itemToSelect = item;
                break;
            }
        }
        if (itemToSelect) {
            stateListWidget->setCurrentItem(itemToSelect);
        } else {
            qValueDetailDisplay->clear();
        }
    } else {
        qValueDetailDisplay->clear();
    }
    LOG_DEFAULT(LogLevel::DEBUG, "QTablePanel: Q-Table içeriği GUI'de güncellendi. Toplam listelenen durum: " << stateListWidget->count());
}

void QTablePanel::handleWorkerError(const QString& error_message) {
    LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "QTablePanel: Worker hatası: " << error_message.toStdString());
    QMessageBox::critical(this, "Hata", "Q-Table verisi çekme sırasında bir hata oluştu: " + error_message);
}

void QTablePanel::onSelectedStateChanged(QListWidgetItem* current, QListWidgetItem* previous) {
    Q_UNUSED(previous);
    if (!current) {
        qValueDetailDisplay->clear();
        return;
    }

    QString selectedStateKey = current->data(Qt::UserRole).toString();
    auto it = displayedQTableDetails.find(selectedStateKey);
    if (it != displayedQTableDetails.end()) {
        displayQValueDetails(it->second);
        LOG_DEFAULT(LogLevel::DEBUG, "QTablePanel: Q-Value detayları gösterildi. StateKey: " << selectedStateKey.toStdString().substr(0, 50) << "...");
    } else {
        qValueDetailDisplay->setText("Detaylar bulunamadı.");
        LOG_DEFAULT(LogLevel::WARNING, "QTablePanel: Seçilen durum anahtarı dahili listeye uymuyor: " << selectedStateKey.toStdString().substr(0, 50) << "...");
    }
}

void QTablePanel::onSearchTextChanged(const QString& text) {
    applyFiltersAndFetchData();
}

void QTablePanel::onClearSearchClicked() {
    searchLineEdit->clear();
    applyFiltersAndFetchData();
}

void QTablePanel::displayQValueDetails(const QTableDisplayData& data) {
    QString details;
    details += "<h3>Q-Table Detayları</h3>";
    details += "<b>Durum Anahtarı (Kısmi):</b> " + data.stateKey.left(500) + "...<br><br>";
    details += "<b>Eylem Q-Değerleri:</b><br>";

    QList<std::pair<QString, float>> sortedQValues;
    for (auto it = data.actionQValues.begin(); it != data.actionQValues.end(); ++it) {
        sortedQValues.append(std::make_pair(it->first, it->second));
    }
    std::sort(sortedQValues.begin(), sortedQValues.end(), [](const std::pair<QString, float>& a, const std::pair<QString, float>& b) {
        return a.second > b.second; // Azalan sırada sırala
    });

    for (const auto& pair : sortedQValues) {
        details += QString(" &nbsp;&nbsp;&nbsp;<b>%1:</b> %2<br>").arg(pair.first).arg(QString::number(pair.second, 'f', 5));
    }

    qValueDetailDisplay->setHtml(details);
}

void QTablePanel::applyFiltersAndFetchData() {
    QString selectedStateKey;
    if (stateListWidget->currentItem()) { selectedStateKey = stateListWidget->currentItem()->data(Qt::UserRole).toString(); }

    emit requestFetchQTableContent(searchLineEdit->text(), selectedStateKey);
}

} // namespace CerebrumLux