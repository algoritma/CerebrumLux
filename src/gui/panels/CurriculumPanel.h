#ifndef CURRICULUM_PANEL_H
#define CURRICULUM_PANEL_H

#include <QWidget>
#include "../../ai_tutor/curriculum.h"

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

    const CerebrumLux::Curriculum& getCurriculum() const;
    void setCurriculum(const CerebrumLux::Curriculum& curriculum);

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

    CerebrumLux::Curriculum m_curriculum;
};

#endif // CURRICULUM_PANEL_H