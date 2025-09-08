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
    addPage(new CourseSelectPage);
    addPage(new LessonPage);
    addPage(new QuizPage);
    addPage(new QuestionsPage);
    addPage(new ConclusionPage);

    setWindowTitle("Material Creation Wizard");
    resize(800, 600); // Make the wizard larger by default
}

void MaterialCreationWizard::accept()
{
    sendDataToServer();
    QDialog::accept();
}

void MaterialCreationWizard::sendDataToServer()
{
    QJsonObject data;
    data["course_id"] = field("course.id").toInt();

    if (field("isLesson").toBool()) {
        data["title"] = field("lesson.title").toString();
        LessonPage *lp = qobject_cast<LessonPage *>(page(Page_Lesson));
        if (lp) {
            data["content"] = lp->content();
        }
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
    return MaterialCreationWizard::Page_CourseSelect;
}

// --- Course Select Page ---
CourseSelectPage::CourseSelectPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle("Select Course");
    setSubTitle("Choose the class and course for this material.");

    auto *layout = new QFormLayout(this);
    m_classCombo = new QComboBox(this);
    m_courseCombo = new QComboBox(this);
    layout->addRow("Class:", m_classCombo);
    layout->addRow("Course:", m_courseCombo);

    registerField("course.id", m_courseCombo, "currentData");

    connect(m_classCombo,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &CourseSelectPage::onClassSelected);
    connect(m_courseCombo, &QComboBox::currentTextChanged, this, &QWizardPage::completeChanged);
}

void CourseSelectPage::initializePage()
{
    NetworkManager::instance()
        .sendCommand("GET_ALL_CLASSES", QJsonObject(), [this](const QJsonObject &response) {
            if (response["type"].toString() == "DATA_RESPONSE") {
                m_classes = response["data"].toArray();
                m_classCombo->clear();
                for (const auto &val : m_classes) {
                    QJsonObject classObj = val.toObject();
                    m_classCombo->addItem(classObj["class_name"].toString(),
                                          classObj["class_id"].toInt());
                }
            }
        });
}

int CourseSelectPage::nextId() const
{
    if (field("isLesson").toBool()) {
        return MaterialCreationWizard::Page_Lesson;
    } else {
        return MaterialCreationWizard::Page_Quiz;
    }
}

bool CourseSelectPage::isComplete() const
{
    return m_courseCombo->currentIndex() != -1;
}

void CourseSelectPage::onClassSelected(int index)
{
    if (index < 0)
        return;

    int classId = m_classCombo->itemData(index).toInt();
    QJsonObject data;
    data["class_id"] = classId;

    NetworkManager::instance()
        .sendCommand("GET_COURSES_FOR_CLASS", data, [this](const QJsonObject &response) {
            if (response["type"].toString() == "DATA_RESPONSE") {
                QJsonArray courses = response["data"].toArray();
                m_courseCombo->clear();
                for (const auto &val : courses) {
                    QJsonObject courseObj = val.toObject();
                    m_courseCombo->addItem(courseObj["course_name"].toString(),
                                           courseObj["course_id"].toInt());
                }
                completeChanged();
            }
        });
}

// --- Lesson Page ---
LessonPage::LessonPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle("Lesson Details");
    setSubTitle("Enter the title and content for the lesson.");

    auto *layout = new QFormLayout(this);
    auto *titleEdit = new QLineEdit(this);
    m_contentEdit = new QTextEdit(this);

    layout->addRow("Title:", titleEdit);
    layout->addRow("Content:", m_contentEdit);

    registerField("lesson.title*", titleEdit);

    connect(titleEdit, &QLineEdit::textChanged, this, &QWizardPage::completeChanged);
}

int LessonPage::nextId() const
{
    return MaterialCreationWizard::Page_Conclusion;
}

QString LessonPage::content() const
{
    return m_contentEdit->toPlainText();
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
    setTitle("Add & Edit Questions");
    setSubTitle(
        "Add questions and their options to the quiz. Select a question from the list to edit it.");

    auto *mainLayout = new QHBoxLayout(this);
    auto *splitter = new QSplitter(Qt::Horizontal, this);

    // Left Panel: Question List
    auto *leftWidget = new QWidget(this);
    auto *leftLayout = new QVBoxLayout(leftWidget);
    m_questionsList = new QListWidget(this);
    m_questionsList->setWordWrap(true);
    leftLayout->addWidget(new QLabel("Questions:", this));
    leftLayout->addWidget(m_questionsList);

    auto *questionButtonsLayout = new QHBoxLayout();
    m_newQuestionButton = new QPushButton("New Question", this);
    m_removeQuestionButton = new QPushButton("Remove Selected", this);
    questionButtonsLayout->addWidget(m_newQuestionButton);
    questionButtonsLayout->addWidget(m_removeQuestionButton);
    leftLayout->addLayout(questionButtonsLayout);
    splitter->addWidget(leftWidget);

    // Right Panel: Question Editor
    auto *rightWidget = new QWidget(this);
    auto *rightLayout = new QVBoxLayout(rightWidget);

    rightLayout->addWidget(new QLabel("Question Editor:", this));
    m_questionPromptEdit = new QTextEdit(this);
    m_questionPromptEdit->setMaximumHeight(150);
    rightLayout->addWidget(m_questionPromptEdit);

    auto *typeLayout = new QHBoxLayout();
    typeLayout->addWidget(new QLabel("Question Type:", this));
    m_questionTypeCombo = new QComboBox(this);
    m_questionTypeCombo->addItems(QStringList() << "radio" << "checkbox" << "open_answer");
    typeLayout->addWidget(m_questionTypeCombo);
    typeLayout->addStretch();
    rightLayout->addLayout(typeLayout);

    // Options table for multiple choice questions
    rightLayout->addWidget(new QLabel("Options (for radio/checkbox):", this));
    m_optionsTable = new QTableWidget(0, 2, this);
    m_optionsTable->setHorizontalHeaderLabels(QStringList() << "Option Text" << "Is Correct");
    m_optionsTable->horizontalHeader()->setStretchLastSection(true);
    rightLayout->addWidget(m_optionsTable);

    auto *optionsButtonLayout = new QHBoxLayout();
    m_addOptionButton = new QPushButton("Add Option", this);
    m_removeOptionButton = new QPushButton("Remove Option", this);
    optionsButtonLayout->addWidget(m_addOptionButton);
    optionsButtonLayout->addWidget(m_removeOptionButton);
    optionsButtonLayout->addStretch();
    rightLayout->addLayout(optionsButtonLayout);
    rightLayout->addStretch();

    m_addOrUpdateQuestionButton = new QPushButton("Add Question", this);
    rightLayout->addWidget(m_addOrUpdateQuestionButton);

    splitter->addWidget(rightWidget);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);
    mainLayout->addWidget(splitter);

    // Connections
    connect(m_newQuestionButton, &QPushButton::clicked, this, &QuestionsPage::onNewQuestion);
    connect(m_addOrUpdateQuestionButton,
            &QPushButton::clicked,
            this,
            &QuestionsPage::onAddOrUpdateQuestion);
    connect(m_removeQuestionButton, &QPushButton::clicked, this, &QuestionsPage::onRemoveQuestion);
    connect(m_questionsList, &QListWidget::itemClicked, this, &QuestionsPage::onQuestionSelected);
    connect(m_questionTypeCombo,
            &QComboBox::currentTextChanged,
            this,
            &QuestionsPage::onQuestionTypeChanged);
    connect(m_addOptionButton, &QPushButton::clicked, this, &QuestionsPage::onAddOption);
    connect(m_removeOptionButton, &QPushButton::clicked, this, &QuestionsPage::onRemoveOption);

    registerField("quiz.questions", this, "questionsJson");

    onNewQuestion(); // Start in "new question" mode
}

bool QuestionsPage::isComplete() const
{
    return m_questions.size() > 0;
}

int QuestionsPage::nextId() const
{
    return MaterialCreationWizard::Page_Conclusion;
}

void QuestionsPage::onNewQuestion()
{
    m_questionsList->setCurrentRow(-1);
    clearEditor();
    m_currentQuestionIndex = -1;
    m_addOrUpdateQuestionButton->setText("Add Question");
    m_questionPromptEdit->setFocus();
}

void QuestionsPage::onAddOrUpdateQuestion()
{
    QJsonObject question = createQuestionFromEditor();
    if (question.isEmpty()) {
        return; // createQuestionFromEditor will show a message box on error
    }

    if (m_currentQuestionIndex == -1) { // Adding a new question
        m_questions.append(question);
        m_questionsList->addItem(question["prompt"].toString());
    } else { // Updating an existing question
        m_questions[m_currentQuestionIndex] = question;
        m_questionsList->item(m_currentQuestionIndex)->setText(question["prompt"].toString());
    }

    onNewQuestion(); // Clear editor for next question
    emit completeChanged();
}

void QuestionsPage::onRemoveQuestion()
{
    int row = m_questionsList->currentRow();
    if (row >= 0) {
        m_questions.removeAt(row);
        delete m_questionsList->takeItem(row);

        onNewQuestion(); // Go back to a clean state
        emit completeChanged();
    }
}

void QuestionsPage::onQuestionSelected(QListWidgetItem *item)
{
    Q_UNUSED(item);
    int row = m_questionsList->currentRow();
    if (row >= 0) {
        loadQuestionForEditing(row);
    }
}

void QuestionsPage::loadQuestionForEditing(int index)
{
    if (index < 0 || index >= m_questions.size())
        return;

    m_currentQuestionIndex = index;
    m_addOrUpdateQuestionButton->setText("Update Question");

    QJsonObject question = m_questions[index].toObject();
    m_questionPromptEdit->setPlainText(question["prompt"].toString());
    m_questionTypeCombo->setCurrentText(question["question_type"].toString());

    onQuestionTypeChanged(m_questionTypeCombo->currentText()); // Enable/disable options table

    m_optionsTable->setRowCount(0); // Clear existing options
    if (question.contains("options")) {
        QJsonArray options = question["options"].toArray();
        for (const QJsonValue &val : options) {
            QJsonObject option = val.toObject();
            int newRow = m_optionsTable->rowCount();
            m_optionsTable->insertRow(newRow);
            m_optionsTable->setItem(newRow, 0, new QTableWidgetItem(option["text"].toString()));
            auto *checkBox = new QCheckBox(this);
            checkBox->setChecked(option["is_correct"].toBool());
            m_optionsTable->setCellWidget(newRow, 1, checkBox);
        }
    }
}

void QuestionsPage::clearEditor()
{
    m_questionPromptEdit->clear();
    m_questionTypeCombo->setCurrentIndex(0);
    m_optionsTable->setRowCount(0);
}

QJsonObject QuestionsPage::createQuestionFromEditor()
{
    if (m_questionPromptEdit->toPlainText().isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Please enter a question prompt.");
        return QJsonObject();
    }

    QJsonObject question;
    question["prompt"] = m_questionPromptEdit->toPlainText();
    question["question_type"] = m_questionTypeCombo->currentText();

    if (m_questionTypeCombo->currentText() != "open_answer") {
        QJsonArray options;
        bool hasCorrectOption = false;
        for (int i = 0; i < m_optionsTable->rowCount(); ++i) {
            QJsonObject option;
            if (!m_optionsTable->item(i, 0) || m_optionsTable->item(i, 0)->text().isEmpty()) {
                QMessageBox::warning(this,
                                     "Validation Error",
                                     QString("Option text for row %1 cannot be empty.").arg(i + 1));
                return QJsonObject();
            }
            option["text"] = m_optionsTable->item(i, 0)->text();
            auto *checkBox = qobject_cast<QCheckBox *>(m_optionsTable->cellWidget(i, 1));
            bool isCorrect = checkBox ? checkBox->isChecked() : false;
            option["is_correct"] = isCorrect;
            if (isCorrect)
                hasCorrectOption = true;
            options.append(option);
        }

        if (m_optionsTable->rowCount() < 2) {
            QMessageBox::warning(this,
                                 "Validation Error",
                                 "Multiple choice questions must have at least two options.");
            return QJsonObject();
        }

        if (!hasCorrectOption) {
            QMessageBox::warning(this,
                                 "Validation Error",
                                 "At least one option must be marked as correct.");
            return QJsonObject();
        }
        question["options"] = options;
    }

    return question;
}

void QuestionsPage::onQuestionTypeChanged(const QString &type)
{
    bool showOptions = (type != "open_answer");
    m_optionsTable->setVisible(showOptions);
    m_addOptionButton->setVisible(showOptions);
    m_removeOptionButton->setVisible(showOptions);

    if (!showOptions) {
        m_optionsTable->setRowCount(0);
    }
}

void QuestionsPage::onAddOption()
{
    int row = m_optionsTable->rowCount();
    m_optionsTable->insertRow(row);
    m_optionsTable->setItem(row, 0, new QTableWidgetItem("New Option"));
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
