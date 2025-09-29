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

#include "../../core/logger.h"
#include "../../learning/KnowledgeBase.h"
#include "../../learning/Capsule.h"
#include "../../learning/LearningModule.h"


namespace CerebrumLux {

// Kapsül detaylarını görüntülemek için yardımcı bir yapı
struct KnowledgeCapsuleDisplayData {
    QString id;
    QString topic;
    QString source;
    QString summary;
    QString fullContent;
    QString cryptofigBlob;
    float confidence;
};

class KnowledgeBasePanel : public QWidget
{
    Q_OBJECT
public:
    explicit KnowledgeBasePanel(LearningModule& learningModuleRef, QWidget *parent = nullptr);
    virtual ~KnowledgeBasePanel();

    void updateKnowledgeBaseContent();

signals:
    // Sinyaller

private slots:
    void onSelectedCapsuleChanged(QListWidgetItem* current, QListWidgetItem* previous);
    void onSearchTextChanged(const QString& text);
    void onClearSearchClicked();
    void onTopicFilterChanged(const QString& topic);
    void onStartDateChanged(const QDate& date);
    void onEndDateChanged(const QDate& date);

private:
    LearningModule& learningModule;

    QListWidget *capsuleListWidget;
    QTextEdit *capsuleDetailDisplay;
    QLineEdit *searchLineEdit;
    QPushButton *clearSearchButton;
    QComboBox *topicFilterComboBox;
    QDateEdit *startDateEdit;
    QDateEdit *endDateEdit;

    std::vector<Capsule> currentDisplayedCapsules;
    std::map<QString, KnowledgeCapsuleDisplayData> displayedCapsuleDetails;
    std::string selectedCapsuleId_;

    void setupUi();
    void displayCapsuleDetails(const KnowledgeCapsuleDisplayData& data);
    void filterAndDisplayCapsules(const QString& filterText = QString(), const QString& topicFilter = QString(), const QDate& startDate = QDate(), const QDate& endDate = QDate());
};

} // namespace CerebrumLux

#endif // KNOWLEDGE_BASE_PANEL_H
