#ifndef KNOWLEDGE_BASE_PANEL_H
#define KNOWLEDGE_BASE_PANEL_H

#include <QWidget>
#include <QListWidget>
#include <QTextEdit>
#include <QSplitter>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QDateEdit>
#include <map>
#include <QThread> // Worker için QThread
#include <vector>
#include <QSet> // QSet için
#include <chrono> // std::chrono::system_clock::time_point için

#include "../../core/logger.h"
#include "../../learning/KnowledgeBase.h" // Doğru yol
#include "../../learning/Capsule.h"     // Doğru yol
#include "../../learning/LearningModule.h" // LearningModule için
#include "KnowledgeBaseWorker.h" // Worker sınıfı için

namespace CerebrumLux {

// Kapsül detaylarını görüntülemek için yardımcı bir yapı
struct KnowledgeCapsuleDisplayData {
    QString id;
    QString topic;
    QString source;
    QString summary;
    QString fullContent;
    QString cryptofigBlob;
    std::vector<float> embedding;
    std::chrono::system_clock::time_point timestamp_utc; // Zaman damgası eklendi
    float confidence;
    QString code_file_path; 
};

class KnowledgeBasePanel : public QWidget
{
    Q_OBJECT
public:
    explicit KnowledgeBasePanel(LearningModule& learningModuleRef, QWidget *parent = nullptr);
    virtual ~KnowledgeBasePanel();
    
    void updateKnowledgeBaseContent(); // Public slot olarak kalıyor

signals:
    // DÜZELTİLDİ: requestFetchAllCapsulesAndDetails sinyali 6 argüman alacak şekilde güncellendi.
    void requestFetchAllCapsulesAndDetails(const QString& filterText, const QString& topicFilter, const QDate& startDate, const QDate& endDate, const QString& specialFilter, const QString& currentSelectionId);
    void requestFetchRelatedCapsules(const std::string& current_capsule_id, const std::vector<float>& current_capsule_embedding);

private slots:
    // Worker'dan gelen sonuçları işleyecek slot'lar.
    void handleAllCapsulesFetched(const std::vector<Capsule>& all_capsules,
                                  const std::map<QString, KnowledgeCapsuleDisplayData>& displayed_details,
                                  const QSet<QString>& unique_topics,
                                  const QString& restoreSelectionId); // Seçimi geri yüklemek için ID
    void handleRelatedCapsulesFetched(const std::vector<Capsule>& related_capsules, const std::string& for_capsule_id);
    void handleWorkerError(const QString& error_message);

    // UI olaylarını işleyecek slot'lar.
    void onSelectedCapsuleChanged(QListWidgetItem* current, QListWidgetItem* previous);
    void onSearchTextChanged(const QString& text);
    void onClearSearchClicked();
    void onTopicFilterChanged(const QString& topic);
    void onSpecialFilterChanged(const QString& filter);
    void onRelatedCapsuleClicked(QListWidgetItem* item);
    void onStartDateChanged(const QDate& date);
    void onEndDateChanged(const QDate& date);
    void onAcceptSuggestionClicked();
    void onRejectSuggestionClicked();

private:
    LearningModule& learningModule;

    QThread workerThread;                 // İşçi iş parçacığı
    KnowledgeBaseWorker *knowledgeBaseWorker; // İşçi nesnesi

    // UI Elemanları
    QListWidget *capsuleListWidget;
    QTextEdit *capsuleDetailDisplay;
    QLineEdit *searchLineEdit;
    QPushButton *clearSearchButton;
    QComboBox *topicFilterComboBox;
    QComboBox *specialFilterComboBox;
    QDateEdit *startDateEdit;
    QDateEdit *endDateEdit;
    QSplitter *mainSplitter; 
    QListWidget *relatedCapsuleListWidget;
    QPushButton *acceptSuggestionButton;
    QPushButton *rejectSuggestionButton;

    // Dahili Veri Yapıları
    std::vector<Capsule> currentDisplayedCapsules;
    std::map<QString, KnowledgeCapsuleDisplayData> displayedCapsuleDetails;
    std::vector<CerebrumLux::Capsule> currentRelatedCapsules;

    void setupUi();
    void updateRelatedCapsules(const std::string& current_capsule_id, const std::vector<float>& current_capsule_embedding);
    void displayCapsuleDetails(const KnowledgeCapsuleDisplayData& data);
    void applyFiltersAndFetchData(); // YENİ EKLENDİ: Tekrar tanımlama hatası olmaması için sadece deklarasyon.
    // DÜZELTİLDİ: filterAndDisplayCapsules metodu artık doğrudan worker'a sinyal emit ediyor.
    void filterAndDisplayCapsules(const QString& filterText = QString(),
                                  const QString& topicFilter = QString(),
                                  const QDate& startDate = QDate(),
                                  const QDate& endDate = QDate(),
                                  const QString& specialFilter = "Tümü");

    void updateSuggestionFeedbackButtons(const QString& selectedCapsuleId);

};

} // namespace CerebrumLux

#endif // KNOWLEDGE_BASE_PANEL_H