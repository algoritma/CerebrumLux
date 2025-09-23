#ifndef CAPSULETRANSFERPANEL_H
#define CAPSULETRANSFERPANEL_H

#include <QWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout> // Main layout
#include <QHBoxLayout> // For buttons and input fields
#include "../../learning/LearningModule.h" // IngestReport için

// Forward declarations for Qt classes
// (QTextEdit, QLineEdit, QPushButton, QVBoxLayout, QHBoxLayout zaten yukarıda dahil edildi)

class CapsuleTransferPanel : public QWidget
{
    Q_OBJECT
public:
    explicit CapsuleTransferPanel(QWidget* parent = nullptr);

signals:
    // Kapsül yutma isteği için sinyal
    void ingestCapsuleRequest(const QString& capsuleJson, const QString& signature, const QString& senderId);
    // Web'den kapsül çekme isteği için sinyal
    void fetchWebCapsuleRequest(const QString& query);

public slots:
    // ingest_envelope'dan gelen raporu göstermek için slot
    void displayIngestReport(const IngestReport& report);

private slots:
    // UI etkileşimleri için slotlar
    void onIngestCapsuleClicked();
    void onFetchFromWebClicked();

private:
    // UI Elemanları
    QTextEdit* jsonInputTextEdit;
    QLineEdit* signatureLineEdit;
    QLineEdit* senderIdLineEdit;
    QPushButton* ingestCapsuleButton;
    QLineEdit* webQueryLineEdit;
    QPushButton* fetchFromWebButton;
    QTextEdit* ingestReportTextEdit;

    // Layout'lar
    QVBoxLayout* mainLayout;
    QHBoxLayout* ingestControlsLayout;
    QHBoxLayout* fetchWebControlsLayout;
};

#endif // CAPSULETRANSFERPANEL_H