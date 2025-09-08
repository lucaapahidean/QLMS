#include "courselistwidget.h"
#include "networkmanager.h"
#include <QButtonGroup>
#include <QCheckBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QSplitter>
#include <QTabWidget>
#include <QTextEdit>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

CourseListWidget::CourseListWidget(int studentId, QWidget *parent)
    : QWidget(parent)
    , m_studentId(studentId)
{
    setupUi();
    onRefresh();
}

void CourseListWidget::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);

    // Create tab widget for materials and attempts
    m_tabWidget = new QTabWidget(this);

    // Materials tab
    auto *materialsWidget = new QWidget(this);
    auto *materialsLayout = new QHBoxLayout(materialsWidget);
    auto *materialsSplitter = new QSplitter(Qt::Horizontal, this);

    auto *leftWidget = new QWidget(this);
    auto *leftLayout = new QVBoxLayout(leftWidget);
    auto *materialsLabel = new QLabel("Available Materials", this);
    QFont font = materialsLabel->font();
    font.setBold(true);
    materialsLabel->setFont(font);
    leftLayout->addWidget(materialsLabel);
    m_materialsFilterEdit = new QLineEdit(this);
    m_materialsFilterEdit->setPlaceholderText("Filter by name or instructor...");
    leftLayout->addWidget(m_materialsFilterEdit);
    m_materialsTreeWidget = new QTreeWidget(this);
    m_materialsTreeWidget->setHeaderLabels({"Name", "Type", "ID", "Instructor"});
    m_materialsTreeWidget->setColumnHidden(2, true);
    m_materialsTreeWidget->setColumnHidden(3, true);
    leftLayout->addWidget(m_materialsTreeWidget);
    m_refreshButton = new QPushButton("Refresh", this);
    leftLayout->addWidget(m_refreshButton);
    materialsSplitter->addWidget(leftWidget);

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
    materialsSplitter->addWidget(rightWidget);
    materialsSplitter->setStretchFactor(0, 1);
    materialsSplitter->setStretchFactor(1, 2);
    materialsLayout->addWidget(materialsSplitter);
    m_tabWidget->addTab(materialsWidget, "Course Materials");

    // Quiz Attempts tab
    auto *attemptsWidget = new QWidget(this);
    auto *attemptsLayout = new QHBoxLayout(attemptsWidget);
    auto *attemptsSplitter = new QSplitter(Qt::Horizontal, this);

    auto *attemptsLeftWidget = new QWidget(this);
    auto *attemptsLeftLayout = new QVBoxLayout(attemptsLeftWidget);
    auto *attemptsLabel = new QLabel("Your Quiz Attempts", this);
    attemptsLabel->setFont(font);
    attemptsLeftLayout->addWidget(attemptsLabel);
    m_attemptsFilterEdit = new QLineEdit(this);
    m_attemptsFilterEdit->setPlaceholderText("Filter by quiz name or instructor...");
    attemptsLeftLayout->addWidget(m_attemptsFilterEdit);
    m_attemptsTreeWidget = new QTreeWidget(this);
    m_attemptsTreeWidget->setHeaderLabels({"Name", "Details", "ID"});
    m_attemptsTreeWidget->setColumnHidden(2, true);
    attemptsLeftLayout->addWidget(m_attemptsTreeWidget);
    m_refreshAttemptsButton = new QPushButton("Refresh", this);
    attemptsLeftLayout->addWidget(m_refreshAttemptsButton);
    attemptsSplitter->addWidget(attemptsLeftWidget);

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
    connect(m_refreshButton, &QPushButton::clicked, this, &CourseListWidget::onRefresh);
    connect(m_materialsTreeWidget,
            &QTreeWidget::itemClicked,
            this,
            &CourseListWidget::onItemSelected);
    connect(m_startQuizButton, &QPushButton::clicked, this, &CourseListWidget::onStartQuiz);
    connect(m_submitQuizButton, &QPushButton::clicked, this, &CourseListWidget::onSubmitQuiz);
    connect(m_refreshAttemptsButton, &QPushButton::clicked, this, &CourseListWidget::onRefresh);
    connect(m_attemptsTreeWidget,
            &QTreeWidget::itemClicked,
            this,
            &CourseListWidget::onAttemptSelected);
    connect(m_materialsFilterEdit,
            &QLineEdit::textChanged,
            this,
            &CourseListWidget::applyMaterialsFilter);
    connect(m_attemptsFilterEdit,
            &QLineEdit::textChanged,
            this,
            &CourseListWidget::applyAttemptsFilter);
}

void CourseListWidget::onRefresh()
{
    m_materialsTreeWidget->clear();
    NetworkManager::instance().sendCommand("GET_ALL_CLASSES",
                                           QJsonObject(),
                                           [this](const QJsonObject &response) {
                                               handleClassesResponse(response);
                                           });

    NetworkManager::instance().sendCommand("GET_MY_ATTEMPTS",
                                           QJsonObject(),
                                           [this](const QJsonObject &response) {
                                               if (response["type"] == "DATA_RESPONSE") {
                                                   m_allAttempts = response["data"].toArray();
                                                   populateAttemptsList();
                                               }
                                           });
}

void CourseListWidget::onItemSelected()
{
    QTreeWidgetItem *item = m_materialsTreeWidget->currentItem();
    if (!item)
        return;

    QString type = item->text(1);
    clearContentArea();

    if (type == "lesson" || type == "quiz") {
        int materialId = item->text(2).toInt();
        QJsonObject data;
        data["material_id"] = materialId;
        NetworkManager::instance()
            .sendCommand("GET_MATERIAL_DETAILS", data, [this, type](const QJsonObject &response) {
                if (response["type"] == "DATA_RESPONSE") {
                    QJsonObject material = response["data"].toObject();
                    if (type == "lesson") {
                        displayTextLesson(material);
                    } else {
                        m_currentQuiz = material;
                        m_contentGroup->setTitle(material["title"].toString());
                        m_startQuizButton->show();
                    }
                }
            });
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
    QTreeWidgetItem *item = m_attemptsTreeWidget->currentItem();
    if (!item || item->text(2).isEmpty())
        return;

    int attemptId = item->text(2).toInt();
    QJsonObject data;
    data["attempt_id"] = attemptId;
    NetworkManager::instance().sendCommand("GET_ATTEMPT_DETAILS",
                                           data,
                                           [this](const QJsonObject &response) {
                                               handleAttemptDetailsResponse(response);
                                           });
}

void CourseListWidget::handleClassesResponse(const QJsonObject &response)
{
    if (response["type"].toString() == "DATA_RESPONSE") {
        QJsonArray classes = response["data"].toArray();
        for (const auto &val : classes) {
            QJsonObject classObj = val.toObject();
            auto *classItem = new QTreeWidgetItem(m_materialsTreeWidget);
            classItem->setText(0, classObj["class_name"].toString());
            classItem->setText(1, "class");

            QJsonObject data;
            data["class_id"] = classObj["class_id"].toInt();
            NetworkManager::instance().sendCommand("GET_COURSES_FOR_CLASS",
                                                   data,
                                                   [this,
                                                    classItem](const QJsonObject &courseResponse) {
                                                       handleCoursesResponse(courseResponse,
                                                                             classItem);
                                                   });
        }
    }
}

void CourseListWidget::handleCoursesResponse(const QJsonObject &response, QTreeWidgetItem *classItem)
{
    if (response["type"].toString() == "DATA_RESPONSE") {
        QJsonArray courses = response["data"].toArray();
        for (const auto &val : courses) {
            QJsonObject courseObj = val.toObject();
            auto *courseItem = new QTreeWidgetItem(classItem);
            courseItem->setText(0, courseObj["course_name"].toString());
            courseItem->setText(1, "course");

            QJsonObject data;
            data["course_id"] = courseObj["course_id"].toInt();
            NetworkManager::instance().sendCommand("GET_MATERIALS_FOR_COURSE",
                                                   data,
                                                   [this, courseItem](
                                                       const QJsonObject &materialResponse) {
                                                       handleMaterialsResponse(materialResponse,
                                                                               courseItem);
                                                   });
        }
    }
}

void CourseListWidget::handleMaterialsResponse(const QJsonObject &response,
                                               QTreeWidgetItem *courseItem)
{
    if (response["type"].toString() == "DATA_RESPONSE") {
        QJsonArray materials = response["data"].toArray();
        for (const auto &val : materials) {
            QJsonObject materialObj = val.toObject();
            auto *materialItem = new QTreeWidgetItem(courseItem);
            materialItem->setText(0, materialObj["title"].toString());
            materialItem->setText(1, materialObj["type"].toString());
            materialItem->setText(2, QString::number(materialObj["material_id"].toInt()));
            materialItem->setText(3, materialObj["instructor_name"].toString());
        }
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
        QMessageBox::information(
            this,
            "Success",
            "Quiz submitted successfully! Check the Quiz History tab to review your attempt.");
        onRefresh();
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
    QString detailsText;
    detailsText += QString("Attempt #%1\n").arg(attemptData["attempt_number"].toInt());
    detailsText += QString("Status: %1\n\n")
                       .arg(attemptData["status"].toString() == "completed"
                                ? "Graded"
                                : "Pending Manual Grading");
    if (!attemptData["auto_score"].isNull()) {
        detailsText += QString("Auto-graded Score: %1%\n")
                           .arg(attemptData["auto_score"].toDouble(), 0, 'f', 1);
    }
    if (!attemptData["final_score"].isNull()) {
        detailsText += QString("Final Score: %1%\n")
                           .arg(attemptData["final_score"].toDouble(), 0, 'f', 1);
    }
    if (attemptData["feedback_type"].toString() != "score_only" && attemptData.contains("answers")) {
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
                if (!correct && attemptData["feedback_type"].toString() == "detailed_with_answers"
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
                if (attemptData["status"].toString() == "completed") {
                    detailsText += QString("Score: %1%\n")
                                       .arg(answer["points_earned"].toDouble() * 100, 0, 'f', 2);
                } else {
                    detailsText += "Result: Pending manual grading\n";
                }
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
    for (auto *widget : m_quizWidgets) {
        widget->deleteLater();
    }
    m_quizWidgets.clear();
    for (auto *child : m_contentGroup->findChildren<QScrollArea *>()) {
        child->deleteLater();
    }
}

void CourseListWidget::populateAttemptsList()
{
    m_attemptsTreeWidget->clear();
    QMap<int, QTreeWidgetItem *> classItems;
    QMap<int, QTreeWidgetItem *> courseItems;
    QMap<int, QTreeWidgetItem *> quizItems;

    for (const auto &val : m_allAttempts) {
        QJsonObject attempt = val.toObject();
        int classId = attempt["class_id"].toInt();
        if (!classItems.contains(classId)) {
            auto *classItem = new QTreeWidgetItem(m_attemptsTreeWidget);
            classItem->setText(0, attempt["class_name"].toString());
            classItems[classId] = classItem;
        }

        int courseId = attempt["course_id"].toInt();
        if (!courseItems.contains(courseId)) {
            auto *courseItem = new QTreeWidgetItem(classItems[classId]);
            courseItem->setText(0, attempt["course_name"].toString());
            courseItems[courseId] = courseItem;
        }

        int quizId = attempt["quiz_id"].toInt();
        if (!quizItems.contains(quizId)) {
            auto *quizItem = new QTreeWidgetItem(courseItems[courseId]);
            quizItem->setText(0, attempt["quiz_title"].toString());
            quizItem->setText(3, attempt["instructor_name"].toString());
            quizItems[quizId] = quizItem;
        }

        auto *attemptItem = new QTreeWidgetItem(quizItems[quizId]);
        QString statusText = (attempt["status"].toString() == "completed") ? "Graded" : "Pending";
        QString text;
        if (attempt["final_score"].isNull()) {
            text = QString("Attempt %1 - %2").arg(attempt["attempt_number"].toInt()).arg(statusText);
        } else {
            text = QString("Attempt %1 - Score: %2% - %3")
                       .arg(attempt["attempt_number"].toInt())
                       .arg(attempt["final_score"].toDouble(), 0, 'f', 1)
                       .arg(statusText);
        }
        attemptItem->setText(0, text);
        attemptItem->setText(2, QString::number(attempt["attempt_id"].toInt()));
    }
    m_attemptsTreeWidget->expandAll();
}

void CourseListWidget::applyMaterialsFilter(const QString &text)
{
    for (int i = 0; i < m_materialsTreeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem *classItem = m_materialsTreeWidget->topLevelItem(i);
        bool classVisible = false;
        for (int j = 0; j < classItem->childCount(); ++j) {
            QTreeWidgetItem *courseItem = classItem->child(j);
            bool courseVisible = false;
            for (int k = 0; k < courseItem->childCount(); ++k) {
                QTreeWidgetItem *materialItem = courseItem->child(k);
                bool match = materialItem->text(0).contains(text, Qt::CaseInsensitive)
                             || materialItem->text(3).contains(text, Qt::CaseInsensitive);
                materialItem->setHidden(!match);
                if (match)
                    courseVisible = true;
            }
            courseItem->setHidden(!courseVisible);
            if (courseVisible)
                classVisible = true;
        }
        classItem->setHidden(!classVisible);
    }
}

void CourseListWidget::applyAttemptsFilter(const QString &text)
{
    for (int i = 0; i < m_attemptsTreeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem *classItem = m_attemptsTreeWidget->topLevelItem(i);
        bool classVisible = false;
        for (int j = 0; j < classItem->childCount(); ++j) {
            QTreeWidgetItem *courseItem = classItem->child(j);
            bool courseVisible = false;
            for (int k = 0; k < courseItem->childCount(); ++k) {
                QTreeWidgetItem *quizItem = courseItem->child(k);
                bool match = quizItem->text(0).contains(text, Qt::CaseInsensitive)
                             || quizItem->text(3).contains(text, Qt::CaseInsensitive);
                quizItem->setHidden(!match);
                if (match)
                    courseVisible = true;
            }
            courseItem->setHidden(!courseVisible);
            if (courseVisible)
                classVisible = true;
        }
        classItem->setHidden(!classVisible);
    }
}
