#include "courselistwidget.h"
#include "networkmanager.h"
#include <QButtonGroup>
#include <QCheckBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QSplitter>
#include <QTabWidget>
#include <QTextEdit>
#include <QVBoxLayout>

CourseListWidget::CourseListWidget(int studentId, QWidget *parent)
    : QWidget(parent)
    , m_studentId(studentId)
{
    setupUi();
    onRefreshMaterials();
    onRefreshAttempts();
}

void CourseListWidget::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);

    // Create tab widget for materials and attempts
    m_tabWidget = new QTabWidget(this);

    // Materials tab
    auto *materialsWidget = new QWidget(this);
    auto *materialsLayout = new QHBoxLayout(materialsWidget);

    auto *splitter = new QSplitter(Qt::Horizontal, this);

    // Left panel - Materials list
    auto *leftWidget = new QWidget(this);
    auto *leftLayout = new QVBoxLayout(leftWidget);

    auto *materialsLabel = new QLabel("Available Materials", this);
    QFont font = materialsLabel->font();
    font.setBold(true);
    materialsLabel->setFont(font);
    leftLayout->addWidget(materialsLabel);

    m_materialsListWidget = new QListWidget(this);
    leftLayout->addWidget(m_materialsListWidget);

    m_refreshButton = new QPushButton("Refresh", this);
    leftLayout->addWidget(m_refreshButton);

    splitter->addWidget(leftWidget);

    // Right panel - Content area
    auto *rightWidget = new QWidget(this);
    auto *rightLayout = new QVBoxLayout(rightWidget);

    m_contentGroup = new QGroupBox("Select a material", this);
    m_contentLayout = new QVBoxLayout(m_contentGroup);

    m_lessonTextEdit = new QTextEdit(this);
    m_lessonTextEdit->setReadOnly(true);
    m_lessonTextEdit->hide();
    m_contentLayout->addWidget(m_lessonTextEdit);

    m_startQuizButton = new QPushButton("Start Quiz", this);
    m_startQuizButton->hide();
    m_contentLayout->addWidget(m_startQuizButton);

    m_submitQuizButton = new QPushButton("Submit Quiz", this);
    m_submitQuizButton->hide();
    m_contentLayout->addWidget(m_submitQuizButton);

    rightLayout->addWidget(m_contentGroup);

    splitter->addWidget(rightWidget);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);

    materialsLayout->addWidget(splitter);
    m_tabWidget->addTab(materialsWidget, "Course Materials");

    // Quiz Attempts tab
    auto *attemptsWidget = new QWidget(this);
    auto *attemptsLayout = new QHBoxLayout(attemptsWidget);

    auto *attemptsSplitter = new QSplitter(Qt::Horizontal, this);

    // Left panel - Attempts list
    auto *attemptsLeftWidget = new QWidget(this);
    auto *attemptsLeftLayout = new QVBoxLayout(attemptsLeftWidget);

    auto *attemptsLabel = new QLabel("Your Quiz Attempts", this);
    attemptsLabel->setFont(font);
    attemptsLeftLayout->addWidget(attemptsLabel);

    m_attemptsListWidget = new QListWidget(this);
    attemptsLeftLayout->addWidget(m_attemptsListWidget);

    m_refreshAttemptsButton = new QPushButton("Refresh", this);
    attemptsLeftLayout->addWidget(m_refreshAttemptsButton);

    attemptsSplitter->addWidget(attemptsLeftWidget);

    // Right panel - Attempt details
    auto *attemptsRightWidget = new QWidget(this);
    auto *attemptsRightLayout = new QVBoxLayout(attemptsRightWidget);

    m_attemptDetailsGroup = new QGroupBox("Select an attempt to review", this);
    m_attemptDetailsLayout = new QVBoxLayout(m_attemptDetailsGroup);

    m_attemptDetailsText = new QTextEdit(this);
    m_attemptDetailsText->setReadOnly(true);
    m_attemptDetailsLayout->addWidget(m_attemptDetailsText);

    attemptsRightLayout->addWidget(m_attemptDetailsGroup);

    attemptsSplitter->addWidget(attemptsRightWidget);
    attemptsSplitter->setStretchFactor(0, 1);
    attemptsSplitter->setStretchFactor(1, 2);

    attemptsLayout->addWidget(attemptsSplitter);
    m_tabWidget->addTab(attemptsWidget, "Quiz History");

    mainLayout->addWidget(m_tabWidget);

    // Connect signals
    connect(m_refreshButton, &QPushButton::clicked, this, &CourseListWidget::onRefreshMaterials);
    connect(m_materialsListWidget,
            &QListWidget::itemClicked,
            this,
            &CourseListWidget::onMaterialSelected);
    connect(m_startQuizButton, &QPushButton::clicked, this, &CourseListWidget::onStartQuiz);
    connect(m_submitQuizButton, &QPushButton::clicked, this, &CourseListWidget::onSubmitQuiz);
    connect(m_refreshAttemptsButton,
            &QPushButton::clicked,
            this,
            &CourseListWidget::onRefreshAttempts);
    connect(m_attemptsListWidget,
            &QListWidget::itemClicked,
            this,
            &CourseListWidget::onAttemptSelected);
}

void CourseListWidget::onRefreshMaterials()
{
    NetworkManager::instance().sendCommand("GET_MATERIALS",
                                           QJsonObject(),
                                           [this](const QJsonObject &response) {
                                               handleMaterialsResponse(response);
                                           });
}

void CourseListWidget::onRefreshAttempts()
{
    QJsonObject data;
    data["student_id"] = m_studentId;

    NetworkManager::instance().sendCommand("GET_MY_ATTEMPTS",
                                           data,
                                           [this](const QJsonObject &response) {
                                               handleAttemptsResponse(response);
                                           });
}

void CourseListWidget::onMaterialSelected()
{
    int row = m_materialsListWidget->currentRow();
    if (row < 0 || row >= m_materials.size())
        return;

    QJsonObject material = m_materials[row].toObject();
    QString type = material["type"].toString();

    clearContentArea();

    if (type == "lesson") {
        displayTextLesson(material);
    } else if (type == "quiz") {
        m_currentQuiz = material;
        m_contentGroup->setTitle(material["title"].toString());
        m_startQuizButton->show();
    }
}

void CourseListWidget::onStartQuiz()
{
    if (m_currentQuiz.isEmpty())
        return;

    int quizId = m_currentQuiz["material_id"].toInt();

    QJsonObject data;
    data["quiz_id"] = quizId;

    NetworkManager::instance().sendCommand("START_QUIZ", data, [this](const QJsonObject &response) {
        handleQuizDataResponse(response);
    });
}

void CourseListWidget::onSubmitQuiz()
{
    if (m_currentQuiz.isEmpty())
        return;

    QJsonArray answers;
    QJsonArray questions = m_currentQuiz["questions"].toArray();

    for (int i = 0; i < questions.size(); ++i) {
        QJsonObject question = questions[i].toObject();
        int questionId = question["question_id"].toInt();
        QString type = question["question_type"].toString();
        QString response;

        if (i < m_quizWidgets.size()) {
            if (type == "radio") {
                auto *group = qobject_cast<QButtonGroup *>(m_quizWidgets[i]);
                if (group && group->checkedButton()) {
                    response = QString::number(group->checkedId());
                }
            } else if (type == "checkbox") {
                auto *container = m_quizWidgets[i];
                QStringList selected;
                int optionIndex = 0;
                for (auto *child : container->findChildren<QCheckBox *>()) {
                    if (child->isChecked()) {
                        selected.append(QString::number(optionIndex));
                    }
                    optionIndex++;
                }
                response = selected.join(",");
            } else if (type == "open_answer") {
                auto *textEdit = qobject_cast<QTextEdit *>(m_quizWidgets[i]);
                if (textEdit) {
                    response = textEdit->toPlainText();
                }
            }
        }

        QJsonObject answer;
        answer["question_id"] = questionId;
        answer["response"] = response;
        answers.append(answer);
    }

    QJsonObject data;
    data["quiz_id"] = m_currentQuiz["material_id"].toInt();
    data["answers"] = answers;

    NetworkManager::instance().sendCommand("FINISH_ATTEMPT",
                                           data,
                                           [this](const QJsonObject &response) {
                                               handleQuizSubmissionResponse(response);
                                           });
}

void CourseListWidget::onAttemptSelected()
{
    int row = m_attemptsListWidget->currentRow();
    if (row < 0 || row >= m_attempts.size())
        return;

    QJsonObject attempt = m_attempts[row].toObject();
    int attemptId = attempt["attempt_id"].toInt();

    QJsonObject data;
    data["attempt_id"] = attemptId;

    NetworkManager::instance().sendCommand("GET_ATTEMPT_DETAILS",
                                           data,
                                           [this](const QJsonObject &response) {
                                               handleAttemptDetailsResponse(response);
                                           });
}

void CourseListWidget::handleMaterialsResponse(const QJsonObject &response)
{
    if (response["type"].toString() == "DATA_RESPONSE") {
        m_materials = response["data"].toArray();
        populateMaterialsList(m_materials);
    } else {
        QMessageBox::critical(this, "Error", "Failed to fetch materials");
    }
}

void CourseListWidget::handleAttemptsResponse(const QJsonObject &response)
{
    if (response["type"].toString() == "DATA_RESPONSE") {
        m_attempts = response["data"].toArray();
        populateAttemptsList(m_attempts);
    }
}

void CourseListWidget::handleQuizDataResponse(const QJsonObject &response)
{
    if (response["type"].toString() == "DATA_RESPONSE") {
        m_currentQuiz = response["data"].toObject();
        displayQuiz(m_currentQuiz);
    } else {
        QMessageBox::critical(this, "Error", response["message"].toString("Failed to load quiz"));
    }
}

void CourseListWidget::handleQuizSubmissionResponse(const QJsonObject &response)
{
    if (response["type"].toString() == "OK") {
        QMessageBox::information(this,
                                 "Success",
                                 "Quiz submitted successfully! "
                                 "Check the Quiz History tab to review your attempt.");

        onRefreshAttempts();
        m_tabWidget->setCurrentIndex(1); // Switch to Quiz History tab

        clearContentArea();
        m_contentGroup->setTitle("Select a material");
    } else {
        QMessageBox::critical(this, "Error", response["message"].toString());
    }
}

void CourseListWidget::handleAttemptDetailsResponse(const QJsonObject &response)
{
    if (response["type"].toString() != "DATA_RESPONSE") {
        return;
    }

    QJsonObject attemptData = response["data"].toObject();
    displayAttemptDetails(attemptData);
}

void CourseListWidget::populateMaterialsList(const QJsonArray &materials)
{
    m_materialsListWidget->clear();

    for (const QJsonValue &value : materials) {
        QJsonObject material = value.toObject();
        QString text
            = QString("[%1] %2").arg(material["type"].toString()).arg(material["title"].toString());
        m_materialsListWidget->addItem(text);
    }
}

void CourseListWidget::populateAttemptsList(const QJsonArray &attempts)
{
    m_attemptsListWidget->clear();

    for (const QJsonValue &value : attempts) {
        QJsonObject attempt = value.toObject();
        QString status = attempt["status"].toString();
        QString statusText = (status == "completed") ? "Graded" : "Pending";

        QString text;
        if (attempt["final_score"].isNull()) {
            text = QString("%1 - Attempt %2 - %3")
                       .arg(attempt["quiz_title"].toString())
                       .arg(attempt["attempt_number"].toInt())
                       .arg(statusText);
        } else {
            text = QString("%1 - Attempt %2 - Score: %3% - %4")
                       .arg(attempt["quiz_title"].toString())
                       .arg(attempt["attempt_number"].toInt())
                       .arg(attempt["final_score"].toDouble(), 0, 'f', 1)
                       .arg(statusText);
        }

        m_attemptsListWidget->addItem(text);
    }
}

void CourseListWidget::displayTextLesson(const QJsonObject &lesson)
{
    m_contentGroup->setTitle(lesson["title"].toString());
    m_lessonTextEdit->setPlainText(lesson["content"].toString());
    m_lessonTextEdit->show();
}

void CourseListWidget::displayQuiz(const QJsonObject &quiz)
{
    clearContentArea();
    m_contentGroup->setTitle(quiz["title"].toString());

    auto *scrollArea = new QScrollArea(this);
    auto *scrollWidget = new QWidget(scrollArea);
    auto *scrollLayout = new QVBoxLayout(scrollWidget);

    QJsonArray questions = quiz["questions"].toArray();

    for (int i = 0; i < questions.size(); ++i) {
        QJsonObject question = questions[i].toObject();

        auto *questionLabel = new QLabel(QString("%1. %2").arg(i + 1).arg(
                                             question["prompt"].toString()),
                                         this);
        questionLabel->setWordWrap(true);
        QFont font = questionLabel->font();
        font.setBold(true);
        scrollLayout->addWidget(questionLabel);

        QString type = question["question_type"].toString();

        if (type == "radio") {
            auto *buttonGroup = new QButtonGroup(this);
            auto *container = new QWidget(this);
            auto *containerLayout = new QVBoxLayout(container);

            QJsonArray options = question["options"].toArray();
            for (int j = 0; j < options.size(); ++j) {
                QJsonObject option = options[j].toObject();
                auto *radioButton = new QRadioButton(option["text"].toString(), this);
                buttonGroup->addButton(radioButton, j);
                containerLayout->addWidget(radioButton);
            }

            scrollLayout->addWidget(container);
            m_quizWidgets.append(buttonGroup);

        } else if (type == "checkbox") {
            auto *container = new QWidget(this);
            auto *containerLayout = new QVBoxLayout(container);

            QJsonArray options = question["options"].toArray();
            for (const QJsonValue &val : options) {
                QJsonObject option = val.toObject();
                auto *checkBox = new QCheckBox(option["text"].toString(), this);
                containerLayout->addWidget(checkBox);
            }

            scrollLayout->addWidget(container);
            m_quizWidgets.append(container);

        } else if (type == "open_answer") {
            auto *textEdit = new QTextEdit(this);
            textEdit->setMaximumHeight(100);
            scrollLayout->addWidget(textEdit);
            m_quizWidgets.append(textEdit);
        }

        scrollLayout->addSpacing(10);
    }

    scrollWidget->setLayout(scrollLayout);
    scrollArea->setWidget(scrollWidget);
    scrollArea->setWidgetResizable(true);

    m_contentLayout->addWidget(scrollArea);

    m_submitQuizButton->show();
    m_contentLayout->addWidget(m_submitQuizButton);
}

void CourseListWidget::displayAttemptDetails(const QJsonObject &attemptData)
{
    m_attemptDetailsGroup->setTitle(attemptData["quiz_title"].toString());

    QString feedbackType = attemptData["feedback_type"].toString();
    QString status = attemptData["status"].toString();

    QString detailsText;
    detailsText += QString("Attempt #%1\n").arg(attemptData["attempt_number"].toInt());
    detailsText += QString("Status: %1\n\n")
                       .arg(status == "completed" ? "Graded" : "Pending Manual Grading");

    // Display scores
    if (!attemptData["auto_score"].isNull()) {
        detailsText += QString("Auto-graded Score: %1%\n")
                           .arg(attemptData["auto_score"].toDouble(), 0, 'f', 1);
    }
    if (!attemptData["final_score"].isNull()) {
        detailsText += QString("Final Score: %1%\n")
                           .arg(attemptData["final_score"].toDouble(), 0, 'f', 1);
    }

    if (feedbackType != "score_only" && attemptData.contains("answers")) {
        detailsText += "\n--- Question Details ---\n\n";

        QJsonArray answers = attemptData["answers"].toArray();
        for (int i = 0; i < answers.size(); ++i) {
            QJsonObject answer = answers[i].toObject();
            detailsText += QString("Question %1: %2\n").arg(i + 1).arg(answer["prompt"].toString());

            QString responseToDisplay = answer.contains("student_response_text")
                                            ? answer["student_response_text"].toString()
                                            : answer["student_response"].toString();
            detailsText += QString("Your answer: %1\n").arg(responseToDisplay);

            if (!answer["is_correct"].isNull()) {
                bool correct = answer["is_correct"].toBool();
                detailsText += QString("Result: %1\n").arg(correct ? "✓ Correct" : "✗ Incorrect");

                if (!correct && feedbackType == "detailed_with_answers"
                    && answer.contains("correct_answers")) {
                    QJsonArray correctAnswers = answer["correct_answers"].toArray();
                    if (!correctAnswers.isEmpty()) {
                        detailsText += "Correct answer(s): ";
                        QStringList correctList;
                        for (const QJsonValue &val : correctAnswers) {
                            correctList.append(val.toString());
                        }
                        detailsText += correctList.join(", ") + "\n";
                    }
                }
            } else if (answer["question_type"].toString() == "open_answer") {
                detailsText += "Result: Pending manual grading\n";
            }

            detailsText += "\n";
        }
    }

    m_attemptDetailsText->setPlainText(detailsText);
}

void CourseListWidget::clearContentArea()
{
    m_lessonTextEdit->clear();
    m_lessonTextEdit->hide();
    m_startQuizButton->hide();
    m_submitQuizButton->hide();

    // Clean up quiz widgets
    for (auto *widget : m_quizWidgets) {
        widget->deleteLater();
    }
    m_quizWidgets.clear();

    // Remove any scroll areas
    for (auto *child : m_contentGroup->findChildren<QScrollArea *>()) {
        child->deleteLater();
    }
}
