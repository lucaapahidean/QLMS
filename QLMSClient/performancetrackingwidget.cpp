#include "performancetrackingwidget.h"
#include "filterwidget.h"
#include "networkmanager.h"
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSplitter>
#include <QTextEdit>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

PerformanceTrackingWidget::PerformanceTrackingWidget(int instructorId, QWidget *parent)
    : QWidget(parent)
    , m_instructorId(instructorId)
{
    setupUi();
    onRefresh();
}

void PerformanceTrackingWidget::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);

    // Header
    auto *headerLayout = new QHBoxLayout();
    auto *titleLabel = new QLabel("Student Performance", this);
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

    // Left panel
    auto *leftWidget = new QWidget(this);
    auto *leftLayout = new QVBoxLayout(leftWidget);
    m_filterWidget = new FilterWidget(this);
    m_filterWidget->setFilterOptions({"Student", "Quiz", "Course"});
    leftLayout->addWidget(m_filterWidget);
    m_treeWidget = new QTreeWidget(this);
    m_treeWidget->setColumnWidth(0, 250);
    m_treeWidget->setHeaderLabels({"Name", "Score", "Attempt", "Type", "ID"});
    m_treeWidget->setColumnHidden(3, true);
    m_treeWidget->setColumnHidden(4, true);
    leftLayout->addWidget(m_treeWidget);
    splitter->addWidget(leftWidget);

    // Right panel
    auto *rightWidget = new QWidget(this);
    auto *rightLayout = new QVBoxLayout(rightWidget);
    m_detailsGroup = new QGroupBox("Select an item to view details", this);
    auto *detailsLayout = new QVBoxLayout(m_detailsGroup);
    m_detailsTextEdit = new QTextEdit(this);
    m_detailsTextEdit->setReadOnly(true);
    detailsLayout->addWidget(m_detailsTextEdit);
    rightLayout->addWidget(m_detailsGroup);
    splitter->addWidget(rightWidget);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);

    mainLayout->addWidget(splitter);

    connect(m_refreshButton, &QPushButton::clicked, this, &PerformanceTrackingWidget::onRefresh);
    connect(m_treeWidget,
            &QTreeWidget::itemClicked,
            this,
            &PerformanceTrackingWidget::onItemSelected);
    connect(m_filterWidget,
            &FilterWidget::filterChanged,
            this,
            &PerformanceTrackingWidget::applyFilter);
}

void PerformanceTrackingWidget::onRefresh()
{
    m_treeWidget->clear();
    NetworkManager::instance().sendCommand("GET_ALL_CLASSES",
                                           QJsonObject(),
                                           [this](const QJsonObject &response) {
                                               handleClassesResponse(response);
                                           });
}

void PerformanceTrackingWidget::onItemSelected()
{
    QTreeWidgetItem *item = m_treeWidget->currentItem();
    if (!item)
        return;

    QString type = item->text(3);
    clearDetailsArea();

    if (type == "class") {
        m_detailsGroup->setTitle(QString("Statistics for %1").arg(item->text(0)));
        int classId = item->data(4, Qt::UserRole).toInt();
        QJsonObject data;
        data["class_id"] = classId;
        NetworkManager::instance()
            .sendCommand("GET_CLASS_STATISTICS", data, [this](const QJsonObject &response) {
                if (response["type"] == "DATA_RESPONSE") {
                    QJsonObject stats = response["data"].toObject();
                    QString text = QString("Overall Class Statistics:\n\n"
                                           "Number of Students: %1\n"
                                           "Average Quiz Score: %2%")
                                       .arg(stats["student_count"].toInt())
                                       .arg(stats["average_score"].toDouble(), 0, 'f', 1);
                    m_detailsTextEdit->setPlainText(text);
                }
            });
    } else if (type == "course") {
        m_detailsGroup->setTitle(QString("Statistics for %1").arg(item->text(0)));
        int courseId = item->data(4, Qt::UserRole).toInt();
        QJsonObject data;
        data["course_id"] = courseId;
        NetworkManager::instance()
            .sendCommand("GET_COURSE_STATISTICS", data, [this](const QJsonObject &response) {
                if (response["type"] == "DATA_RESPONSE") {
                    QJsonObject stats = response["data"].toObject();
                    QString text = QString("Overall Course Statistics:\n\n"
                                           "Active Students (with attempts): %1\n"
                                           "Average Quiz Score: %2%")
                                       .arg(stats["student_count"].toInt())
                                       .arg(stats["average_score"].toDouble(), 0, 'f', 1);
                    m_detailsTextEdit->setPlainText(text);
                }
            });
    } else if (type == "quiz") {
        int quizId = item->data(4, Qt::UserRole).toInt();
        QJsonObject data;
        data["quiz_id"] = quizId;
        NetworkManager::instance().sendCommand(
            "GET_STUDENT_ATTEMPTS_FOR_QUIZ", data, [this, item](const QJsonObject &response) {
                if (response["type"] == "DATA_RESPONSE") {
                    qDeleteAll(item->takeChildren());
                    QJsonArray attempts = response["data"].toArray();
                    for (const auto &val : attempts) {
                        QJsonObject attempt = val.toObject();
                        auto *attemptItem = new QTreeWidgetItem(item);
                        QString scoreText = attempt["final_score"].isNull()
                                                ? "Pending"
                                                : QString::number(attempt["final_score"].toDouble(),
                                                                  'f',
                                                                  1)
                                                      + "%";
                        attemptItem->setText(0, attempt["student_name"].toString());
                        attemptItem->setText(1, scoreText);
                        attemptItem->setText(2, QString::number(attempt["attempt_number"].toInt()));
                        attemptItem->setText(3, "attempt");
                        attemptItem->setData(4, Qt::UserRole, attempt["attempt_id"].toInt());
                    }
                    item->setExpanded(true);
                }
            });
    } else if (type == "attempt") {
        int attemptId = item->data(4, Qt::UserRole).toInt();
        QJsonObject data;
        data["attempt_id"] = attemptId;
        NetworkManager::instance().sendCommand("GET_ATTEMPT_DETAILS",
                                               data,
                                               [this](const QJsonObject &response) {
                                                   handleAttemptDetailsResponse(response);
                                               });
    }
}

void PerformanceTrackingWidget::handleClassesResponse(const QJsonObject &response)
{
    if (response["type"].toString() == "DATA_RESPONSE") {
        QJsonArray classes = response["data"].toArray();
        for (const auto &val : classes) {
            QJsonObject classObj = val.toObject();
            auto *classItem = new QTreeWidgetItem(m_treeWidget);
            classItem->setText(0, classObj["class_name"].toString());
            classItem->setText(3, "class");
            classItem->setData(4, Qt::UserRole, classObj["class_id"].toInt());

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

void PerformanceTrackingWidget::handleCoursesResponse(const QJsonObject &response,
                                                      QTreeWidgetItem *classItem)
{
    if (response["type"].toString() == "DATA_RESPONSE") {
        QJsonArray courses = response["data"].toArray();
        for (const auto &val : courses) {
            QJsonObject courseObj = val.toObject();
            auto *courseItem = new QTreeWidgetItem(classItem);
            courseItem->setText(0, courseObj["course_name"].toString());
            courseItem->setText(3, "course");
            courseItem->setData(4, Qt::UserRole, courseObj["course_id"].toInt());

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

void PerformanceTrackingWidget::handleMaterialsResponse(const QJsonObject &response,
                                                        QTreeWidgetItem *courseItem)
{
    if (response["type"].toString() == "DATA_RESPONSE") {
        QJsonArray materials = response["data"].toArray();
        for (const auto &val : materials) {
            QJsonObject materialObj = val.toObject();
            if (materialObj["type"].toString() == "quiz") {
                auto *materialItem = new QTreeWidgetItem(courseItem);
                materialItem->setText(0, materialObj["title"].toString());
                materialItem->setText(3, "quiz");
                materialItem->setData(4, Qt::UserRole, materialObj["material_id"].toInt());
            }
        }
    }
}

void PerformanceTrackingWidget::handleAttemptDetailsResponse(const QJsonObject &response)
{
    if (response["type"].toString() != "DATA_RESPONSE") {
        m_detailsTextEdit->setPlainText("Could not load attempt details.");
        return;
    }
    QJsonObject attemptData = response["data"].toObject();
    displayAttemptDetails(attemptData);
}

void PerformanceTrackingWidget::displayAttemptDetails(const QJsonObject &attemptData)
{
    m_detailsGroup->setTitle(
        QString("Details for %1's attempt at %2")
            .arg(m_treeWidget->currentItem()->text(0), attemptData["quiz_title"].toString()));
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
    if (attemptData.contains("answers")) {
        detailsText += "\n--- Question Details ---\n\n";
        QJsonArray answers = attemptData["answers"].toArray();
        for (int i = 0; i < answers.size(); ++i) {
            QJsonObject answer = answers[i].toObject();
            detailsText += QString("Question %1: %2\n").arg(i + 1).arg(answer["prompt"].toString());
            QString responseToDisplay = answer.contains("student_response_text")
                                            ? answer["student_response_text"].toString()
                                            : answer["student_response"].toString();
            detailsText += QString("Student's answer: %1\n").arg(responseToDisplay);

            if (!answer["is_correct"].isNull()) {
                bool correct = answer["is_correct"].toBool();
                detailsText += QString("Result: %1\n").arg(correct ? "✓ Correct" : "✗ Incorrect");
                if (!correct && answer.contains("correct_answers")) {
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
                if (attemptData["status"].toString() == "completed"
                    && !answer["points_earned"].isNull()) {
                    detailsText += QString("Score: %1%\n")
                                       .arg(answer["points_earned"].toDouble() * 100, 0, 'f', 2);
                } else {
                    detailsText += "Result: Pending manual grading\n";
                }
            }
            detailsText += "\n";
        }
    }
    m_detailsTextEdit->setPlainText(detailsText);
}

void PerformanceTrackingWidget::clearDetailsArea()
{
    m_detailsGroup->setTitle("Select a student's attempt to view details");
    m_detailsTextEdit->clear();
}

void PerformanceTrackingWidget::applyFilter()
{
    for (int i = 0; i < m_treeWidget->topLevelItemCount(); ++i) {
        applyFilterRecursive(m_treeWidget->topLevelItem(i),
                             m_filterWidget->filterText(),
                             m_filterWidget->currentFilterOption());
    }
}

bool PerformanceTrackingWidget::applyFilterRecursive(QTreeWidgetItem *item,
                                                     const QString &text,
                                                     const QString &filterBy)
{
    bool anyChildMatches = false;
    for (int i = 0; i < item->childCount(); ++i) {
        if (applyFilterRecursive(item->child(i), text, filterBy)) {
            anyChildMatches = true;
        }
    }

    bool selfMatches = false;
    QString itemType = item->text(3); // Type is in column 3
    if ((filterBy == "Student" && itemType == "attempt")
        || (filterBy == "Quiz" && itemType == "quiz")
        || (filterBy == "Course" && itemType == "course")) {
        selfMatches = item->text(0).contains(text, Qt::CaseInsensitive);
    }

    bool shouldBeVisible = selfMatches || anyChildMatches;
    item->setHidden(!shouldBeVisible);

    return shouldBeVisible;
}
