#ifndef Q_TABLE_PANEL_H
#define Q_TABLE_PANEL_H

#include <QWidget>
#include <QListWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QSplitter>

#include "../../core/logger.h"
#include "../../learning/LearningModule.h"
#include "../../swarm_vectordb/DataModels.h" // SparseQTable ve EmbeddingStateKey için
#include "../../core/enums.h" // AIAction için
#include "../../core/utils.h" // action_to_string için

namespace CerebrumLux {

// Q-Table detaylarını görüntülemek için yardımcı bir yapı
struct QTableDisplayData {
    QString stateKey;
    std::map<QString, float> actionQValues; // Eylem adı -> Q-Value
};

class QTablePanel : public QWidget
{
    Q_OBJECT
public:
    explicit QTablePanel(LearningModule& learningModuleRef, QWidget *parent = nullptr);
    virtual ~QTablePanel();
    
    void updateQTableContent(); // Q-Table içeriğini güncelleyen public metod

private slots:
    void onSelectedStateChanged(QListWidgetItem* current, QListWidgetItem* previous);
    void onSearchTextChanged(const QString& text);
    void onClearSearchClicked();

private:
    LearningModule& learningModule;

    QListWidget *stateListWidget;      // Tüm durum anahtarlarını listeler
    QTextEdit *qValueDetailDisplay;    // Seçilen durumun Q-değerlerini detaylı gösterir
    QLineEdit *searchLineEdit;         // Durum anahtarlarında arama yapmak için
    QPushButton *clearSearchButton;    // Arama kutusunu temizlemek için
    QSplitter *splitter;               // Liste ve detay görünümünü ayırmak için

    std::map<QString, QTableDisplayData> displayedQTableDetails; // Görüntülenen Q-Table verileri
    std::vector<CerebrumLux::SwarmVectorDB::EmbeddingStateKey> currentQStateKeys; // LMDB'den alınan tüm durum anahtarları

    void setupUi();
    void displayQValueDetails(const QTableDisplayData& data);
    void filterAndDisplayStates(const QString& filterText = QString());
};

} // namespace CerebrumLux

#endif // Q_TABLE_PANEL_H