#ifndef Q_TABLE_PANEL_H
#define Q_TABLE_PANEL_H

#include <QWidget>
#include <QListWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QSplitter>
#include <QThread> // Worker için QThread

#include "../../core/logger.h"
#include "../../learning/LearningModule.h"
#include "../../swarm_vectordb/DataModels.h" // SparseQTable ve EmbeddingStateKey için
#include "../../core/enums.h" // AIAction için
#include "../../core/utils.h" // action_to_string için
#include "QTableWorker.h" // Worker sınıfı için
#include "../DataTypes.h" // YENİ EKLENDİ: QTableDisplayData tanımı için

namespace CerebrumLux {

class QTablePanel : public QWidget
{
    Q_OBJECT
public:
    explicit QTablePanel(LearningModule& learningModuleRef, QWidget *parent = nullptr);
    virtual ~QTablePanel();
    
    void updateQTableContent(); // Q-Table içeriğini güncelleyen public metod

signals:
    void requestFetchQTableContent(const QString& filterText, const QString& currentSelectionStateKey);

private slots:
    void handleQTableContentFetched(const std::vector<CerebrumLux::SwarmVectorDB::EmbeddingStateKey>& all_q_state_keys,
                                    const std::map<QString, QTableDisplayData>& displayed_q_table_details,
                                    const QString& restoreSelectionStateKey);
    void handleWorkerError(const QString& error_message);

    void onSelectedStateChanged(QListWidgetItem* current, QListWidgetItem* previous);
    void onSearchTextChanged(const QString& text);
    void onClearSearchClicked();

private:
    LearningModule& learningModule;
    QThread workerThread;
    QTableWorker *qTableWorker;

    QListWidget *stateListWidget;      // Tüm durum anahtarlarını listeler
    QTextEdit *qValueDetailDisplay;    // Seçilen durumın Q-değerlerini detaylı gösterir
    QLineEdit *searchLineEdit;         // Durum anahtarlarında arama yapmak için
    QPushButton *clearSearchButton;    // Arama kutusunu temizlemek için
    QSplitter *splitter;               // Liste ve detay görünümünü ayırmak için

    std::map<QString, QTableDisplayData> displayedQTableDetails; // Görüntülenen Q-Table verileri
    std::vector<CerebrumLux::SwarmVectorDB::EmbeddingStateKey> currentQStateKeys; // LMDB'den alınan tüm durum anahtarları

    void setupUi(); // Kullanıcı arayüzünü başlatan yardımcı metod
    void displayQValueDetails(const QTableDisplayData& data);
    void applyFiltersAndFetchData(); // Tüm filtreleri uygulayıp worker'a veri çekme isteği gönderen yardımcı metod
};

} // namespace CerebrumLux

#endif // Q_TABLE_PANEL_H