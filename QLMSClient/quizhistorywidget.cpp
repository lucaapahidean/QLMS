#include "quizhistorywidget.h"
#include "filterwidget.h"
#include "networkmanager.h"
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>
#include <QTextEdit>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

QuizHistoryWidget::QuizHistoryWidget(int studentId, QWidget *parent)
    : QWidget(parent)
    , m_studentId(studentId)
{
    setupUi();
    onRefresh();
}

void QuizHistoryWidget::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);

    // Header
    auto *headerLayout = new QHBoxLayout();
    auto *titleLabel = new QLabel("Quiz History", this);
    QFont font = titleLabel->font();
    font.setPointSize(14);
    font.setBold(true);
    titleLabel->setFont(font);
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    m_refreshButton = new QPushButton("Refresh", this);
    headerLayout->addWidget(m_refreshButton);
    mainLayout->addLayout(headerLayout);

    auto *splitter = new QSplitter(Qt::Horizontal, this);

    // Left Panel
    auto *leftWidget = new QWidget(this);
    auto *leftLayout = new QVBoxLayout(leftWidget);
    m_attemptsFilterWidget = new FilterWidget(this);
    m_attemptsFilterWidget->setFilterOptions({"Name", "Instructor"});
    leftLayout->addWidget(m_attemptsFilterWidget);
    m_attemptsTreeWidget = new QTreeWidget(this);
    m_attemptsTreeWidget->setColumnWidth(0, 350);
    m_attemptsTreeWidget->setHeaderLabels({"Name", "Score", "Status", "ID", "Instructor"});
    m_attemptsTreeWidget->setColumnHidden(3, true);
    m_attemptsTreeWidget->setColumnHidden(4, true);
    leftLayout->addWidget(m_attemptsTreeWidget);
    splitter->addWidget(leftWidget);

    // Right Panel
    auto *rightWidget = new QWidget(this);
    auto *rightLayout = new QVBoxLayout(rightWidget);
    m_attemptDetailsGroup = new QGroupBox("Select an attempt to review", this);
    auto *detailsLayout = new QVBoxLayout(m_attemptDetailsGroup);
    m_attemptDetailsText = new QTextEdit(this);
    m_attemptDetailsText->setReadOnly(true);
    detailsLayout->addWidget(m_attemptDetailsText);
    rightLayout->addWidget(m_attemptDetailsGroup);
    splitter->addWidget(rightWidget);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);

    mainLayout->addWidget(splitter);

    // Connect signals
    connect(m_refreshButton, &QPushButton::clicked, this, &QuizHistoryWidget::onRefresh);
    connect(m_attemptsTreeWidget,
            &QTreeWidget::itemClicked,
            this,
            &QuizHistoryWidget::onAttemptSelected);
    connect(m_attemptsFilterWidget,
            &FilterWidget::filterChanged,
            this,
            &QuizHistoryWidget::applyFilter);
}

void QuizHistoryWidget::onRefresh()
{
    NetworkManager::instance().sendCommand("GET_MY_ATTEMPTS",
                                           QJsonObject(),
                                           [this](const QJsonObject &response) {
                                               if (response["type"] == "DATA_RESPONSE") {
                                                   m_allAttempts = response["data"].toArray();
                                                   populateAttemptsList();
                                               }
                                           });
}

void QuizHistoryWidget::onAttemptSelected()
{
    QTreeWidgetItem *item = m_attemptsTreeWidget->currentItem();
    if (!item || item->text(3).isEmpty())
        return;

    int attemptId = item->text(3).toInt();
    QJsonObject data;
    data["attempt_id"] = attemptId;
    NetworkManager::instance().sendCommand("GET_ATTEMPT_DETAILS",
                                           data,
                                           [this](const QJsonObject &response) {
                                               handleAttemptDetailsResponse(response);
                                           });
}

void QuizHistoryWidget::handleAttemptDetailsResponse(const QJsonObject &response)
{
    if (response["type"].toString() != "DATA_RESPONSE") {
        return;
    }
    QJsonObject attemptData = response["data"].toObject();
    displayAttemptDetails(attemptData);
}

void QuizHistoryWidget::displayAttemptDetails(const QJsonObject &attemptData)
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

void QuizHistoryWidget::populateAttemptsList()
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
            quizItem->setText(4, attempt["instructor_name"].toString());
            quizItems[quizId] = quizItem;
        }

        auto *attemptItem = new QTreeWidgetItem(quizItems[quizId]);
        QString statusText = (attempt["status"].toString() == "completed") ? "Graded" : "Pending";
        QString scoreText = "N/A";
        if (!attempt["final_score"].isNull()) {
            scoreText = QString("%1%").arg(attempt["final_score"].toDouble(), 0, 'f', 1);
        }

        attemptItem->setText(0, QString("Attempt %1").arg(attempt["attempt_number"].toInt()));
        attemptItem->setText(1, scoreText);
        attemptItem->setText(2, statusText);
        attemptItem->setText(3, QString::number(attempt["attempt_id"].toInt()));
    }
    m_attemptsTreeWidget->expandAll();
    for (int i = 0; i < m_attemptsTreeWidget->columnCount(); ++i) {
        m_attemptsTreeWidget->resizeColumnToContents(i);
    }
}

void QuizHistoryWidget::applyFilter()
{
    for (int i = 0; i < m_attemptsTreeWidget->topLevelItemCount(); ++i) {
        applyFilterRecursive(m_attemptsTreeWidget->topLevelItem(i),
                             m_attemptsFilterWidget->filterText(),
                             m_attemptsFilterWidget->currentFilterOption());
    }
}

bool QuizHistoryWidget::applyFilterRecursive(QTreeWidgetItem *item,
                                             const QString &text,
                                             const QString &filterBy)
{
    bool anyChildMatches = false;
    for (int i = 0; i < item->childCount(); ++i) {
        if (applyFilterRecursive(item->child(i), text, filterBy)) {
            anyChildMatches = true;
        }
    }

    int column = (filterBy == "Name") ? 0 : 4;
    bool selfMatches = item->text(column).contains(text, Qt::CaseInsensitive);
    bool shouldBeVisible = selfMatches || anyChildMatches;
    item->setHidden(!shouldBeVisible);

    return shouldBeVisible;
}
