#ifndef TUTOR_PANEL_H
#define TUTOR_PANEL_H

#include <QWidget>

class QPushButton;
class QPlainTextEdit;
class QLabel;

class TutorPanel : public QWidget {
    Q_OBJECT

public:
    explicit TutorPanel(QWidget *parent = nullptr);
    ~TutorPanel();

public slots:
    void handleTrainingUpdate(const QString& update);
    void handleTrainingFinished();

signals:
    void startTrainingClicked();
    void stopTrainingClicked();

private:
    QPushButton* m_startButton;
    QPushButton* m_stopButton;
    QPlainTextEdit* m_trainingLog;
    QLabel* m_statusLabel;
};

#endif // TUTOR_PANEL_H