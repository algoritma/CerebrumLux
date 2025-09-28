#ifndef KNOWLEDGE_BASE_PANEL_H
#define KNOWLEDGE_BASE_PANEL_H

#include <QWidget>
#include <QListWidget>
#include <QTextEdit>
#include <QSplitter>
#include <QLineEdit> // Arama/süzme için
#include <QPushButton> // Temizle butonu için
#include <map> // std::map için
#include <vector> // std::vector için

#include "../../core/logger.h" // LOG_DEFAULT için
#include "../../learning/KnowledgeBase.h" // CerebrumLux::KnowledgeBase için
#include "../../learning/Capsule.h" // CerebrumLux::Capsule için
#include "../../learning/LearningModule.h" // Semantic search için


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
    // Diğer detaylar eklenebilir
};

class KnowledgeBasePanel : public QWidget
{
    Q_OBJECT
public:
    explicit KnowledgeBasePanel(LearningModule& learningModuleRef, QWidget *parent = nullptr);
    virtual ~KnowledgeBasePanel();

    void updateKnowledgeBaseContent(); // YENİ: KnowledgeBase içeriğini günceller

signals:
    // Gelecekte KnowledgeBase ile ilgili olaylar için sinyaller eklenebilir (örn. kapsül silindi, karantinaya alındı)

private slots:
    void onSelectedCapsuleChanged(QListWidgetItem* current, QListWidgetItem* previous);
    void onSearchTextChanged(const QString& text); // Arama/süzme slotu
    void onClearSearchClicked(); // Arama kutusunu temizle butonu için

private:
    LearningModule& learningModule; // KnowledgeBase'e erişim için

    QListWidget *capsuleListWidget;
    QTextEdit *capsuleDetailDisplay;
    QLineEdit *searchLineEdit; // YENİ: Arama kutusu
    QPushButton *clearSearchButton; // YENİ: Arama kutusunu temizleme butonu

    std::vector<Capsule> currentDisplayedCapsules; // Şu anda görüntülenen tüm kapsüller
    std::map<QString, KnowledgeCapsuleDisplayData> displayedCapsuleDetails; // Detayları saklar

    void setupUi();
    void displayCapsuleDetails(const KnowledgeCapsuleDisplayData& data);
    void filterAndDisplayCapsules(const QString& filterText);
};

} // namespace CerebrumLux

#endif // KNOWLEDGE_BASE_PANEL_H