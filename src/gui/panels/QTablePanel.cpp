#include "QTablePanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMap> // QMap<QString, float> için
#include <algorithm> // std::min için
#include <utility> // std::pair, std::make_pair için
#include <map> // std::map iteratörleri için

namespace CerebrumLux {

QTablePanel::QTablePanel(LearningModule& learningModuleRef, QWidget *parent)
    : QWidget(parent),
      learningModule(learningModuleRef)
{
    setupUi();
    LOG_DEFAULT(LogLevel::INFO, "QTablePanel: Initialized.");
    updateQTableContent(); // Başlangıçta içeriği yükle
}

QTablePanel::~QTablePanel() {
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
    
    QString selectedStateKey;
    if (stateListWidget->currentItem()) {
        selectedStateKey = stateListWidget->currentItem()->data(Qt::UserRole).toString();
    }

    currentQStateKeys = learningModule.getKnowledgeBase().get_swarm_db().get_all_keys_for_dbi(learningModule.getKnowledgeBase().get_swarm_db().q_values_dbi());
    
    filterAndDisplayStates(searchLineEdit->text()); // Mevcut filtreyle listeyi güncelle

    // Seçimi geri yükle
    if (!selectedStateKey.isEmpty()) {
        QListWidgetItem *itemToSelect = nullptr;
        for (int i = 0; i < stateListWidget->count(); ++i) {
            QListWidgetItem *item = stateListWidget->item(i);
            if (item->data(Qt::UserRole).toString() == selectedStateKey) {
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
    LOG_DEFAULT(LogLevel::DEBUG, "QTablePanel: Q-Table içeriği güncellendi. Toplam durum: " << currentQStateKeys.size());
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
    filterAndDisplayStates(text);
}

void QTablePanel::onClearSearchClicked() {
    searchLineEdit->clear();
    filterAndDisplayStates();
}

void QTablePanel::displayQValueDetails(const QTableDisplayData& data) {
    QString details;
    details += "<h3>Q-Table Detayları</h3>";
    details += "<b>Durum Anahtarı (Kısmi):</b> " + data.stateKey.left(100) + "...<br><br>";
    details += "<b>Eylem Q-Değerleri:</b><br>";

    // DÜZELTİLDİ: std::map iteratörlerinin first ve second üyeleri kullanıldı.
    QList<std::pair<QString, float>> sortedQValues;
    for (auto it = data.actionQValues.begin(); it != data.actionQValues.end(); ++it) {
        sortedQValues.append(std::make_pair(it->first, it->second)); // DÜZELTİLDİ: it->first ve it->second kullanıldı.
    }
    std::sort(sortedQValues.begin(), sortedQValues.end(), [](const std::pair<QString, float>& a, const std::pair<QString, float>& b) {
        return a.second > b.second; // Azalan sırada sırala
    });

    for (const auto& pair : sortedQValues) {
        details += QString(" &nbsp;&nbsp;&nbsp;<b>%1:</b> %2<br>").arg(pair.first).arg(QString::number(pair.second, 'f', 5));
    }

    qValueDetailDisplay->setHtml(details);
}

void QTablePanel::filterAndDisplayStates(const QString& filterText) {
    stateListWidget->clear();
    displayedQTableDetails.clear();

    LOG_DEFAULT(LogLevel::TRACE, "QTablePanel: Durum anahtarı filtreleme baslatildi. Toplam anahtar: " << currentQStateKeys.size() << ", Filtre Metni: '" << filterText.toStdString() << "'");

    for (const auto& state_key_std_str : currentQStateKeys) {
        QString stateKey = QString::fromStdString(state_key_std_str);

        bool matchesSearch = (filterText.isEmpty() || stateKey.contains(filterText, Qt::CaseInsensitive));
        if (!matchesSearch) {
            continue;
        }

        // Q-Table'dan ilgili durumun eylem-Q değerlerini çek
        auto action_map_json_str_opt = learningModule.getKnowledgeBase().get_swarm_db().get_q_value_json(state_key_std_str);

        if (action_map_json_str_opt) {
            try {
                nlohmann::json action_map_json = nlohmann::json::parse(*action_map_json_str_opt);
                
                QTableDisplayData data;
                data.stateKey = stateKey;
                for (nlohmann::json::const_iterator action_it = action_map_json.begin(); action_it != action_map_json.end(); ++action_it) {
                    data.actionQValues[QString::fromStdString(action_it.key())] = action_it.value().get<float>();
                }
                displayedQTableDetails[stateKey] = data;

                QString itemText = QString("Durum Anahtarı: %1 | Eylem Sayısı: %2")
                                    .arg(stateKey.left(50) + "...") // Kısmi görünüm
                                    .arg(data.actionQValues.size());
                
                QListWidgetItem *item = new QListWidgetItem(itemText);
                item->setData(Qt::UserRole, stateKey);
                stateListWidget->addItem(item);
            } catch (const nlohmann::json::exception& e) {
                LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "QTablePanel: JSON ayrıştırma hatası (filterAndDisplayStates): " << e.what() << ". StateKey (kısmi): " << stateKey.toStdString().substr(0, 50) << "...");
            }
        }
    }
    LOG_DEFAULT(LogLevel::DEBUG, "QTablePanel: filterAndDisplayStates tamamlandi. Toplam listelenen durum: " << stateListWidget->count());
}

} // namespace CerebrumLux