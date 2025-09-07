#include "courseeditorwidget.h"
#include "networkmanager.h"
#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QSplitter>
#include <QTableWidget>
#include <QTextEdit>
#include <QVBoxLayout>

CourseEditorWidget::CourseEditorWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
    onRefreshMaterials();
}

void CourseEditorWidget::setupUi()
{
    auto *mainLayout = new QHBoxLayout(this);

    auto *splitter = new QSplitter(Qt::Horizontal, this);

    // Left panel - Materials list
    auto *leftWidget = new QWidget(this);
    auto *leftLayout = new QVBoxLayout(leftWidget);

    auto *materialsLabel = new QLabel("Course Materials", this);
    QFont font = materialsLabel->font();
    font.setBold(true);
    materialsLabel->setFont(font);
    leftLayout->addWidget(materialsLabel);

    m_materialsListWidget = new QListWidget(this);
    leftLayout->addWidget(m_materialsListWidget);

    m_refreshButton = new QPushButton("Refresh", this);
    leftLayout->addWidget(m_refreshButton);

    splitter->addWidget(leftWidget);

    // Right panel - Quiz editor
    auto *rightWidget = new QWidget(this);
    auto *rightLayout = new QVBoxLayout(rightWidget);

    // Quiz creation group
    auto *quizGroup = new QGroupBox("Create New Quiz", this);
    auto *quizLayout = new QGridLayout(quizGroup);

    quizLayout->addWidget(new QLabel("Title:", this), 0, 0);
    m_quizTitleEdit = new QLineEdit(this);
    quizLayout->addWidget(m_quizTitleEdit, 0, 1);

    quizLayout->addWidget(new QLabel("Max Attempts:", this), 1, 0);
    m_maxAttemptsSpinBox = new QSpinBox(this);
    m_maxAttemptsSpinBox->setRange(1, 10);
    m_maxAttemptsSpinBox->setValue(1);
    quizLayout->addWidget(m_maxAttemptsSpinBox, 1, 1);

    quizLayout->addWidget(new QLabel("Feedback Type:", this), 2, 0);
    m_feedbackTypeCombo = new QComboBox(this);
    m_feedbackTypeCombo->addItem("Show answers and explanations", "detailed_with_answers");
    m_feedbackTypeCombo->addItem("Show right/wrong only", "detailed_without_answers");
    m_feedbackTypeCombo->addItem("Show score only", "score_only");
    m_feedbackTypeCombo->setToolTip(
        "Controls what students see after submitting the quiz:\n"
        "• Show answers: Students see which answers were wrong and the correct answers\n"
        "• Right/wrong only: Students see which answers were wrong but not the correct answers\n"
        "• Score only: Students only see their total score");
    quizLayout->addWidget(m_feedbackTypeCombo, 2, 1);

    m_createQuizButton = new QPushButton("Create Quiz", this);
    quizLayout->addWidget(m_createQuizButton, 3, 0, 1, 2);

    rightLayout->addWidget(quizGroup);

    // Question creation group
    auto *questionGroup = new QGroupBox("Add Question to Quiz", this);
    auto *questionLayout = new QVBoxLayout(questionGroup);

    auto *quizIdLayout = new QHBoxLayout();
    quizIdLayout->addWidget(new QLabel("Quiz ID:", this));
    m_currentQuizIdEdit = new QLineEdit(this);
    quizIdLayout->addWidget(m_currentQuizIdEdit);
    questionLayout->addLayout(quizIdLayout);

    questionLayout->addWidget(new QLabel("Question Prompt:", this));
    m_questionPromptEdit = new QTextEdit(this);
    m_questionPromptEdit->setMaximumHeight(100);
    questionLayout->addWidget(m_questionPromptEdit);

    auto *typeLayout = new QHBoxLayout();
    typeLayout->addWidget(new QLabel("Question Type:", this));
    m_questionTypeCombo = new QComboBox(this);
    m_questionTypeCombo->addItems(QStringList() << "radio" << "checkbox" << "open_answer");
    typeLayout->addWidget(m_questionTypeCombo);
    typeLayout->addStretch();
    questionLayout->addLayout(typeLayout);

    // Options table for multiple choice questions
    questionLayout->addWidget(new QLabel("Options (for multiple choice):", this));
    m_optionsTable = new QTableWidget(0, 2, this);
    m_optionsTable->setHorizontalHeaderLabels(QStringList() << "Option Text" << "Is Correct");
    m_optionsTable->horizontalHeader()->setStretchLastSection(true);
    m_optionsTable->setMaximumHeight(150);
    questionLayout->addWidget(m_optionsTable);

    auto *optionsButtonLayout = new QHBoxLayout();
    m_addOptionButton = new QPushButton("Add Option", this);
    m_removeOptionButton = new QPushButton("Remove Option", this);
    optionsButtonLayout->addWidget(m_addOptionButton);
    optionsButtonLayout->addWidget(m_removeOptionButton);
    optionsButtonLayout->addStretch();
    questionLayout->addLayout(optionsButtonLayout);

    m_addQuestionButton = new QPushButton("Add Question", this);
    questionLayout->addWidget(m_addQuestionButton);

    rightLayout->addWidget(questionGroup);
    rightLayout->addStretch();

    splitter->addWidget(rightWidget);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);

    mainLayout->addWidget(splitter);

    // Connect signals
    connect(m_refreshButton, &QPushButton::clicked, this, &CourseEditorWidget::onRefreshMaterials);
    connect(m_createQuizButton, &QPushButton::clicked, this, &CourseEditorWidget::onCreateQuiz);
    connect(m_addQuestionButton, &QPushButton::clicked, this, &CourseEditorWidget::onAddQuestion);
    connect(m_questionTypeCombo,
            &QComboBox::currentTextChanged,
            this,
            &CourseEditorWidget::onQuestionTypeChanged);
    connect(m_addOptionButton, &QPushButton::clicked, this, &CourseEditorWidget::onAddOption);
    connect(m_removeOptionButton, &QPushButton::clicked, this, &CourseEditorWidget::onRemoveOption);

    onQuestionTypeChanged(m_questionTypeCombo->currentText());
}

void CourseEditorWidget::onRefreshMaterials()
{
    NetworkManager::instance().sendCommand("GET_MATERIALS",
                                           QJsonObject(),
                                           [this](const QJsonObject &response) {
                                               handleMaterialsResponse(response);
                                           });
}

void CourseEditorWidget::onCreateQuiz()
{
    if (m_quizTitleEdit->text().isEmpty()) {
        QMessageBox::warning(this, "Create Quiz", "Please enter a quiz title");
        return;
    }

    QJsonObject data;
    data["title"] = m_quizTitleEdit->text();
    data["max_attempts"] = m_maxAttemptsSpinBox->value();
    data["feedback_type"] = m_feedbackTypeCombo->currentData().toString();

    NetworkManager::instance().sendCommand("CREATE_QUIZ", data, [this](const QJsonObject &response) {
        if (response["type"].toString() == "OK") {
            int quizId = response["quiz_id"].toInt();
            m_currentQuizIdEdit->setText(QString::number(quizId));
            QMessageBox::information(this,
                                     "Success",
                                     QString("Quiz created successfully with ID: %1").arg(quizId));
            m_quizTitleEdit->clear();
            onRefreshMaterials();
        } else {
            QMessageBox::critical(this, "Error", response["message"].toString());
        }
    });
}

void CourseEditorWidget::onAddQuestion()
{
    if (m_currentQuizIdEdit->text().isEmpty() || m_questionPromptEdit->toPlainText().isEmpty()) {
        QMessageBox::warning(this, "Add Question", "Please enter quiz ID and question prompt");
        return;
    }

    QJsonObject data;
    data["quiz_id"] = m_currentQuizIdEdit->text().toInt();
    data["prompt"] = m_questionPromptEdit->toPlainText();
    data["question_type"] = m_questionTypeCombo->currentText();

    // Add options for multiple choice questions
    if (m_questionTypeCombo->currentText() != "open_answer") {
        QJsonArray options;
        bool hasCorrectOption = false;

        for (int i = 0; i < m_optionsTable->rowCount(); ++i) {
            QJsonObject option;
            option["text"] = m_optionsTable->item(i, 0)->text();
            auto *checkBox = qobject_cast<QCheckBox *>(m_optionsTable->cellWidget(i, 1));
            bool isCorrect = checkBox ? checkBox->isChecked() : false;
            option["is_correct"] = isCorrect;
            if (isCorrect)
                hasCorrectOption = true;
            options.append(option);
        }

        if (!hasCorrectOption) {
            QMessageBox::warning(this,
                                 "Add Question",
                                 "At least one option must be marked as correct");
            return;
        }

        data["options"] = options;
    }

    NetworkManager::instance().sendCommand("ADD_QUESTION", data, [this](const QJsonObject &response) {
        if (response["type"].toString() == "OK") {
            QMessageBox::information(this, "Success", "Question added successfully");
            clearQuestionForm();
        } else {
            QMessageBox::critical(this, "Error", response["message"].toString());
        }
    });
}

void CourseEditorWidget::onQuestionTypeChanged(const QString &type)
{
    bool showOptions = (type != "open_answer");
    m_optionsTable->setEnabled(showOptions);
    m_addOptionButton->setEnabled(showOptions);
    m_removeOptionButton->setEnabled(showOptions);

    if (!showOptions) {
        m_optionsTable->setRowCount(0);
    }
}

void CourseEditorWidget::onAddOption()
{
    int row = m_optionsTable->rowCount();
    m_optionsTable->insertRow(row);
    m_optionsTable->setItem(row, 0, new QTableWidgetItem(""));

    auto *checkBox = new QCheckBox(this);
    m_optionsTable->setCellWidget(row, 1, checkBox);
}

void CourseEditorWidget::onRemoveOption()
{
    int row = m_optionsTable->currentRow();
    if (row >= 0) {
        m_optionsTable->removeRow(row);
    }
}

void CourseEditorWidget::handleMaterialsResponse(const QJsonObject &response)
{
    if (response["type"].toString() == "DATA_RESPONSE") {
        populateMaterialsList(response["data"].toArray());
    } else {
        QMessageBox::critical(this, "Error", "Failed to fetch materials");
    }
}

void CourseEditorWidget::populateMaterialsList(const QJsonArray &materials)
{
    m_materialsListWidget->clear();

    for (const QJsonValue &value : materials) {
        QJsonObject material = value.toObject();
        QString text = QString("[%1] %2 - %3")
                           .arg(material["material_id"].toInt())
                           .arg(material["type"].toString())
                           .arg(material["title"].toString());
        m_materialsListWidget->addItem(text);
    }
}

void CourseEditorWidget::clearQuestionForm()
{
    m_questionPromptEdit->clear();
    m_optionsTable->setRowCount(0);
}
