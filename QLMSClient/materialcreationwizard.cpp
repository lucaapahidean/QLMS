#include "materialcreationwizard.h"
#include "networkmanager.h"
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QSplitter>
#include <QTableWidget>
#include <QTextEdit>
#include <QVBoxLayout>

MaterialCreationWizard::MaterialCreationWizard(QWidget *parent)
    : QWizard(parent)
{
    addPage(new IntroPage);
    addPage(new LessonPage);
    addPage(new QuizPage);
    addPage(new QuestionsPage);
    addPage(new ConclusionPage);

    setWindowTitle("Material Creation Wizard");
}

void MaterialCreationWizard::accept()
{
    sendDataToServer();
    QDialog::accept();
}

void MaterialCreationWizard::sendDataToServer()
{
    QJsonObject data;

    if (field("isLesson").toBool()) {
        data["title"] = field("lesson.title").toString();
        data["content"] = field("lesson.content").toString();
        NetworkManager::instance()
            .sendCommand("CREATE_LESSON", data, [](const QJsonObject &response) {
                if (response["type"].toString() != "OK") {
                    QMessageBox::critical(nullptr, "Error", response["message"].toString());
                }
            });
    } else { // quiz
        data["title"] = field("quiz.title").toString();
        data["max_attempts"] = field("quiz.max_attempts").toInt();
        data["feedback_type"] = field("quiz.feedback_type").toString();
        data["questions"] = qvariant_cast<QJsonArray>(field("quiz.questions"));

        NetworkManager::instance()
            .sendCommand("CREATE_QUIZ_WITH_QUESTIONS", data, [](const QJsonObject &response) {
                if (response["type"].toString() != "OK") {
                    QMessageBox::critical(nullptr, "Error", response["message"].toString());
                }
            });
    }
}

// --- Intro Page ---
IntroPage::IntroPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle("Select Material Type");
    setSubTitle("Choose whether you want to create a text lesson or a quiz.");

    auto *layout = new QVBoxLayout(this);
    auto *lessonRadio = new QRadioButton("Text Lesson", this);
    auto *quizRadio = new QRadioButton("Quiz", this);
    lessonRadio->setChecked(true);

    layout->addWidget(lessonRadio);
    layout->addWidget(quizRadio);

    registerField("isLesson", lessonRadio);
}

int IntroPage::nextId() const
{
    if (field("isLesson").toBool()) {
        return MaterialCreationWizard::Page_Lesson;
    } else {
        return MaterialCreationWizard::Page_Quiz;
    }
}

// --- Lesson Page ---
LessonPage::LessonPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle("Lesson Details");
    setSubTitle("Enter the title and content for the lesson.");

    auto *layout = new QFormLayout(this);
    auto *titleEdit = new QLineEdit(this);
    auto *contentEdit = new QTextEdit(this);

    layout->addRow("Title:", titleEdit);
    layout->addRow("Content:", contentEdit);

    registerField("lesson.title*", titleEdit);
    registerField("lesson.content*", contentEdit);

    connect(contentEdit, &QTextEdit::textChanged, this, &QWizardPage::completeChanged);
}

int LessonPage::nextId() const
{
    return MaterialCreationWizard::Page_Conclusion;
}

// --- Quiz Page ---
QuizPage::QuizPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle("Quiz Details");
    setSubTitle("Enter the general details for the quiz.");

    auto *layout = new QFormLayout(this);

    auto *titleEdit = new QLineEdit(this);
    layout->addRow("Title:", titleEdit);

    auto *maxAttemptsSpinBox = new QSpinBox(this);
    maxAttemptsSpinBox->setRange(1, 10);
    maxAttemptsSpinBox->setValue(1);
    layout->addRow("Max Attempts:", maxAttemptsSpinBox);

    auto *feedbackTypeCombo = new QComboBox(this);
    feedbackTypeCombo->addItem("Show answers and explanations", "detailed_with_answers");
    feedbackTypeCombo->addItem("Show right/wrong only", "detailed_without_answers");
    feedbackTypeCombo->addItem("Show score only", "score_only");
    layout->addRow("Feedback Type:", feedbackTypeCombo);

    registerField("quiz.title*", titleEdit);
    registerField("quiz.max_attempts", maxAttemptsSpinBox);
    registerField("quiz.feedback_type", feedbackTypeCombo, "currentData");
}

int QuizPage::nextId() const
{
    return MaterialCreationWizard::Page_Questions;
}

// --- Questions Page ---
QuestionsPage::QuestionsPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle("Add Questions");
    setSubTitle("Add questions and their options to the quiz.");

    auto *mainLayout = new QHBoxLayout(this);
    auto *splitter = new QSplitter(Qt::Horizontal, this);

    // Left Panel: Question List
    auto *leftWidget = new QWidget(this);
    auto *leftLayout = new QVBoxLayout(leftWidget);
    m_questionsList = new QListWidget(this);
    leftLayout->addWidget(new QLabel("Questions:", this));
    leftLayout->addWidget(m_questionsList);

    auto *questionButtonsLayout = new QHBoxLayout();
    m_addQuestionButton = new QPushButton("Add Question", this);
    m_removeQuestionButton = new QPushButton("Remove Question", this);
    questionButtonsLayout->addWidget(m_addQuestionButton);
    questionButtonsLayout->addWidget(m_removeQuestionButton);
    leftLayout->addLayout(questionButtonsLayout);
    splitter->addWidget(leftWidget);

    // Right Panel: Question Editor
    auto *rightWidget = new QWidget(this);
    auto *rightLayout = new QVBoxLayout(rightWidget);

    rightLayout->addWidget(new QLabel("Question Prompt:", this));
    m_questionPromptEdit = new QTextEdit(this);
    m_questionPromptEdit->setMaximumHeight(100);
    rightLayout->addWidget(m_questionPromptEdit);

    auto *typeLayout = new QHBoxLayout();
    typeLayout->addWidget(new QLabel("Question Type:", this));
    m_questionTypeCombo = new QComboBox(this);
    m_questionTypeCombo->addItems(QStringList() << "radio" << "checkbox" << "open_answer");
    typeLayout->addWidget(m_questionTypeCombo);
    typeLayout->addStretch();
    rightLayout->addLayout(typeLayout);

    // Options table for multiple choice questions
    rightLayout->addWidget(new QLabel("Options (for multiple choice):", this));
    m_optionsTable = new QTableWidget(0, 2, this);
    m_optionsTable->setHorizontalHeaderLabels(QStringList() << "Option Text" << "Is Correct");
    m_optionsTable->horizontalHeader()->setStretchLastSection(true);
    m_optionsTable->setMaximumHeight(150);
    rightLayout->addWidget(m_optionsTable);

    auto *optionsButtonLayout = new QHBoxLayout();
    m_addOptionButton = new QPushButton("Add Option", this);
    m_removeOptionButton = new QPushButton("Remove Option", this);
    optionsButtonLayout->addWidget(m_addOptionButton);
    optionsButtonLayout->addWidget(m_removeOptionButton);
    optionsButtonLayout->addStretch();
    rightLayout->addLayout(optionsButtonLayout);
    splitter->addWidget(rightWidget);

    mainLayout->addWidget(splitter);

    // Connections
    connect(m_addQuestionButton, &QPushButton::clicked, this, &QuestionsPage::onAddQuestion);
    connect(m_removeQuestionButton, &QPushButton::clicked, this, &QuestionsPage::onRemoveQuestion);
    connect(m_questionTypeCombo,
            &QComboBox::currentTextChanged,
            this,
            &QuestionsPage::onQuestionTypeChanged);
    connect(m_addOptionButton, &QPushButton::clicked, this, &QuestionsPage::onAddOption);
    connect(m_removeOptionButton, &QPushButton::clicked, this, &QuestionsPage::onRemoveOption);

    registerField("quiz.questions", this, "questionsJson");

    onQuestionTypeChanged(m_questionTypeCombo->currentText());
}

bool QuestionsPage::isComplete() const
{
    return m_questions.size() > 0;
}

int QuestionsPage::nextId() const
{
    return MaterialCreationWizard::Page_Conclusion;
}

void QuestionsPage::onAddQuestion()
{
    if (m_questionPromptEdit->toPlainText().isEmpty()) {
        QMessageBox::warning(this, "Add Question", "Please enter a question prompt.");
        return;
    }

    QJsonObject question;
    question["prompt"] = m_questionPromptEdit->toPlainText();
    question["question_type"] = m_questionTypeCombo->currentText();

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
                                 "At least one option must be marked as correct.");
            return;
        }
        question["options"] = options;
    }

    m_questions.append(question);
    m_questionsList->addItem(m_questionPromptEdit->toPlainText());
    m_questionPromptEdit->clear();
    m_optionsTable->setRowCount(0);
    emit completeChanged();
}

void QuestionsPage::onRemoveQuestion()
{
    int row = m_questionsList->currentRow();
    if (row >= 0) {
        m_questions.removeAt(row);
        delete m_questionsList->takeItem(row);
        emit completeChanged();
    }
}

void QuestionsPage::onQuestionTypeChanged(const QString &type)
{
    bool showOptions = (type != "open_answer");
    m_optionsTable->setEnabled(showOptions);
    m_addOptionButton->setEnabled(showOptions);
    m_removeOptionButton->setEnabled(showOptions);

    if (!showOptions) {
        m_optionsTable->setRowCount(0);
    }
}

void QuestionsPage::onAddOption()
{
    int row = m_optionsTable->rowCount();
    m_optionsTable->insertRow(row);
    m_optionsTable->setItem(row, 0, new QTableWidgetItem(""));

    auto *checkBox = new QCheckBox(this);
    m_optionsTable->setCellWidget(row, 1, checkBox);
}

void QuestionsPage::onRemoveOption()
{
    int row = m_optionsTable->currentRow();
    if (row >= 0) {
        m_optionsTable->removeRow(row);
    }
}

// --- Conclusion Page ---
ConclusionPage::ConclusionPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle("Conclusion");
    setSubTitle("Review the details below and click 'Finish' to create the material.");
}

void ConclusionPage::initializePage()
{
    QString summary;
    if (field("isLesson").toBool()) {
        summary = "You are creating a new Text Lesson with the following details:\n\n";
        summary += "Title: " + field("lesson.title").toString() + "\n";
    } else {
        summary = "You are creating a new Quiz with the following details:\n\n";
        summary += "Title: " + field("quiz.title").toString() + "\n";
        summary += "Max Attempts: " + field("quiz.max_attempts").toString() + "\n";
        summary += "Feedback Type: " + field("quiz.feedback_type").toString() + "\n";
        summary += "Number of Questions: "
                   + QString::number(qvariant_cast<QJsonArray>(field("quiz.questions")).size())
                   + "\n";
    }

    auto *label = findChild<QLabel *>();
    if (!label) {
        label = new QLabel(this);
        label->setWordWrap(true);
        auto *layout = new QVBoxLayout(this);
        layout->addWidget(label);
    }
    label->setText(summary);
}
