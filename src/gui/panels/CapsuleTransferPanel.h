#ifndef CAPSULETRANSFERPANEL_H
#define CAPSULETRANSFERPANEL_H

#include <QWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>

#include "../../learning/LearningModule.h" // CerebrumLux::IngestReport için

namespace CerebrumLux { // Namespace içine alındı

class CapsuleTransferPanel : public QWidget
{
    Q_OBJECT
public:
    explicit CapsuleTransferPanel(QWidget *parent = nullptr);
    void displayIngestReport(const IngestReport& report); // CerebrumLux::IngestReport olarak düzeltildi

signals:
    void ingestCapsuleRequest(const QString& capsuleJson, const QString& signature, const QString& senderId);
    void fetchWebCapsuleRequest(const QString& query);

private slots:
    void onIngestButtonClicked();
    void onFetchWebButtonClicked();

private:
    QTextEdit *capsuleJsonInput;
    QLineEdit *signatureInput;
    QLineEdit *senderIdInput;
    QPushButton *ingestButton;
    QLineEdit *webQueryInput;
    QPushButton *fetchWebButton;
    QTextEdit *reportOutput;
};

} // namespace CerebrumLux

#endif // CAPSULETRANSFERPANEL_H