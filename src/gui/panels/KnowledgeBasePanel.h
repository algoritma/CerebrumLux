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
#include <vector>
#include <set> // QSet yerine std::set kullanıldı

#include "../../core/logger.h"
#include "../../learning/KnowledgeBase.h" // Doğru yol
#include "../../learning/Capsule.h"     // Doğru yol
#include "../../learning/LearningModule.h"


namespace CerebrumLux { // Namespace CerebrumLux olarak bırakıldı

// Kapsül detaylarını görüntülemek için yardımcı bir yapı
struct KnowledgeCapsuleDisplayData {
    QString id;
    QString topic;
    QString source;
    QString summary;
    QString fullContent;
    QString cryptofigBlob;
    float confidence;
    QString code_file_path; // ✅ Birleştirildi ve tek bir alan olarak korundu
};

class KnowledgeBasePanel : public QWidget
{
    Q_OBJECT
public:
    explicit KnowledgeBasePanel(LearningModule& learningModuleRef, QWidget *parent = nullptr);
    virtual ~KnowledgeBasePanel();

    void updateKnowledgeBaseContent(); // Public slot olarak kalıyor

signals:
    // Sinyaller

private slots:
    void onSelectedCapsuleChanged(QListWidgetItem* current, QListWidgetItem* previous);
    void onSearchTextChanged(const QString& text);
    void onClearSearchClicked();
    // ✅ DEĞİŞTİRİLDİ: const QString& parametresi alacak şekilde
    void onTopicFilterChanged(const QString& topic);
    // ✅ YENİ SLOT: CodeDevelopment filtre kontrolü için (const QString& parametresi alacak şekilde)
    void onSpecialFilterChanged(const QString& filter);
    void onStartDateChanged(const QDate& date);
    void onEndDateChanged(const QDate& date);

    // Geri bildirim butonları
    void onAcceptSuggestionClicked();
    void onRejectSuggestionClicked();

private:
    LearningModule& learningModule;

    QListWidget *capsuleListWidget;
    QTextEdit *capsuleDetailDisplay;
    QLineEdit *searchLineEdit;
    QPushButton *clearSearchButton;
    QComboBox *topicFilterComboBox;
    QComboBox *specialFilterComboBox;   // ✅ Yeni kontrol
    QDateEdit *startDateEdit;
    QDateEdit *endDateEdit;

    // Geri bildirim butonları
    QPushButton *acceptSuggestionButton;
    QPushButton *rejectSuggestionButton;

    std::vector<Capsule> currentDisplayedCapsules;
    std::map<QString, KnowledgeCapsuleDisplayData> displayedCapsuleDetails;
    //std::string selectedCapsuleId_; // Seçili kapsül ID'si std::string olarak tutulur

    void setupUi();
    void displayCapsuleDetails(const KnowledgeCapsuleDisplayData& data);
    // ✅ DEĞİŞTİRİLDİ: specialFilter parametresi eklendi ve varsayılan değeri var
    void filterAndDisplayCapsules(const QString& filterText = QString(),
                                  const QString& topicFilter = QString(),
                                  const QDate& startDate = QDate(),
                                  const QDate& endDate = QDate(),
                                  const QString& specialFilter = "Tümü"); // Varsayılan değeri eklendi

    void updateSuggestionFeedbackButtons(const QString& selectedCapsuleId);

};

} // namespace CerebrumLux

#endif // KNOWLEDGE_BASE_PANEL_H