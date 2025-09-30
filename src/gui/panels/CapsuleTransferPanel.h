#ifndef CAPSULE_TRANSFER_PANEL_H
#define CAPSULE_TRANSFER_PANEL_H

#include <QWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QLabel>
#include <QComboBox>
#include <QVBoxLayout>
#include <QListWidget>
#include <QSplitter>
#include <map> // std::map için
#include <vector> // std::vector için

#include "../../core/enums.h"
#include "../../learning/LearningModule.h"


namespace CerebrumLux {

struct CapsuleDisplayData {
    QString id;
    QString topic;
    QString source;
    QString summary;
    QString fullContent;
    QString cryptofigBlob;
    float confidence;
};

class CapsuleTransferPanel : public QWidget
{
    Q_OBJECT
public:
    explicit CapsuleTransferPanel(QWidget *parent = nullptr);
    virtual ~CapsuleTransferPanel(); // YENİ: Yıkıcı deklarasyonuna 'virtual' eklendi
    void displayIngestReport(const IngestReport& report);

signals:
    void ingestCapsuleRequest(const QString& capsuleJson, const QString& signature, const QString& senderId);
    void fetchWebCapsuleRequest(const QString& query);

private slots:
    void onIngestCapsuleClicked();
    void onFetchWebCapsuleClicked();

private:
    QLabel *capsuleJsonLabel;
    QTextEdit *capsuleJsonInput;
    QLabel *signatureLabel;
    QLineEdit *signatureInput;
    QLabel *senderIdLabel;
    QLineEdit *senderIdInput;
    QPushButton *ingestCapsuleButton;
    
    QLabel *webQueryLabel;
    QLineEdit *webQueryInput;
    QPushButton *fetchWebCapsuleButton;

    QLabel *reportStatusLabel;
    QTextEdit *reportMessageDisplay;
    
    void setupUi();
};

} // namespace CerebrumLux

#endif // CAPSULE_TRANSFER_PANEL_H