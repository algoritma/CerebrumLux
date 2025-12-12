#include "CurriculumPanel.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QMessageBox>
#include <QStringList>

CurriculumPanel::CurriculumPanel(QWidget *parent) : QWidget(parent) {
    setupUi();
    
    // Varsayılan Müfredat (Demo için)
    Curriculum defaultCurr;
    defaultCurr.sections["coding_cpp"].topics = {"Classes", "Pointers", "Lambda"};
    defaultCurr.sections["coding_cpp"].difficulty = "adaptive";
    defaultCurr.sections["logic"].topics = {"Reasoning", "Math"};
    defaultCurr.sections["logic"].difficulty = "adaptive";
    setCurriculum(defaultCurr);
}

CurriculumPanel::~CurriculumPanel() {}

void CurriculumPanel::setupUi() {
    auto mainLayout = new QVBoxLayout(this);

    auto formLayout = new QFormLayout();
    m_sectionNameInput = new QLineEdit(this);
    m_sectionNameInput->setPlaceholderText("Örn: history");
    formLayout->addRow("Alan Adı:", m_sectionNameInput);

    m_topicsInput = new QTextEdit(this);
    m_topicsInput->setPlaceholderText("Konular (virgül ile ayırın)");
    m_topicsInput->setFixedHeight(50);
    formLayout->addRow("Konular:", m_topicsInput);

    m_difficultyComboBox = new QComboBox(this);
    m_difficultyComboBox->addItems({"adaptive", "easy", "medium", "hard"});
    formLayout->addRow("Zorluk:", m_difficultyComboBox);
    
    m_addButton = new QPushButton("Ekle / Güncelle", this);
    connect(m_addButton, &QPushButton::clicked, this, &CurriculumPanel::addSection);
    formLayout->addWidget(m_addButton);

    m_curriculumTable = new QTableWidget(this);
    m_curriculumTable->setColumnCount(3);
    m_curriculumTable->setHorizontalHeaderLabels({"Alan", "Konular", "Zorluk"});
    m_curriculumTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_curriculumTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_curriculumTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_curriculumTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    m_removeButton = new QPushButton("Seçili Alanı Sil", this);
    connect(m_removeButton, &QPushButton::clicked, this, &CurriculumPanel::removeSelectedSection);

    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(m_curriculumTable);
    mainLayout->addWidget(m_removeButton);
}

void CurriculumPanel::addSection() {
    QString name = m_sectionNameInput->text().trimmed();
    QString topicsStr = m_topicsInput->toPlainText().trimmed();

    if (name.isEmpty() || topicsStr.isEmpty()) {
        QMessageBox::warning(this, "Hata", "Alan adı ve konular gerekli.");
        return;
    }

    QStringList qTopics = topicsStr.split(',', Qt::SkipEmptyParts);
    std::vector<std::string> topics;
    for (const auto& t : qTopics) topics.push_back(t.trimmed().toStdString());

    CurriculumSection section;
    section.topics = topics;
    section.difficulty = m_difficultyComboBox->currentText().toStdString();

    m_curriculum.sections[name.toStdString()] = section;
    
    updateTable();
    m_sectionNameInput->clear();
    m_topicsInput->clear();
}

void CurriculumPanel::removeSelectedSection() {
    auto items = m_curriculumTable->selectedItems();
    if (items.isEmpty()) return;
    
    int row = m_curriculumTable->row(items.first());
    QString key = m_curriculumTable->item(row, 0)->text();
    m_curriculum.sections.erase(key.toStdString());
    updateTable();
}

void CurriculumPanel::updateTable() {
    m_curriculumTable->setRowCount(0);
    for (const auto& [name, sec] : m_curriculum.sections) {
        int r = m_curriculumTable->rowCount();
        m_curriculumTable->insertRow(r);
        m_curriculumTable->setItem(r, 0, new QTableWidgetItem(QString::fromStdString(name)));
        
        QString tList;
        for (size_t i=0; i<sec.topics.size(); i++) {
            tList += QString::fromStdString(sec.topics[i]);
            if (i < sec.topics.size()-1) tList += ", ";
        }
        
        auto tItem = new QTableWidgetItem(tList);
        tItem->setToolTip(tList);
        m_curriculumTable->setItem(r, 1, tItem);
        m_curriculumTable->setItem(r, 2, new QTableWidgetItem(QString::fromStdString(sec.difficulty)));
    }
}

Curriculum CurriculumPanel::getCurriculum() const { return m_curriculum; }
void CurriculumPanel::setCurriculum(const Curriculum& c) { m_curriculum = c; updateTable(); }