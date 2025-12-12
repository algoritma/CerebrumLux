#ifndef CURRICULUM_PANEL_H
#define CURRICULUM_PANEL_H

#include <QWidget>
#include "../../learning/ai_tutor_loop.h"

class QLineEdit;
class QTextEdit;
class QComboBox;
class QPushButton;
class QTableWidget;

class CurriculumPanel : public QWidget {
    Q_OBJECT

public:
    explicit CurriculumPanel(QWidget *parent = nullptr);
    ~CurriculumPanel();

    Curriculum getCurriculum() const;
    void setCurriculum(const Curriculum& curriculum);

private slots:
    void addSection();
    void removeSelectedSection();

private:
    void setupUi();
    void updateTable();

    QLineEdit* m_sectionNameInput;
    QTextEdit* m_topicsInput;
    QComboBox* m_difficultyComboBox;
    QPushButton* m_addButton;
    QPushButton* m_removeButton;
    QTableWidget* m_curriculumTable;

    Curriculum m_curriculum;
};

#endif // CURRICULUM_PANEL_H