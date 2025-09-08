#include "courselistwidget.h"
#include "filterwidget.h"
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

    // Header
    auto *headerLayout = new QHBoxLayout();
    auto *titleLabel = new QLabel("Course Materials", this);
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
    m_materialsFilterWidget = new FilterWidget(this);
    m_materialsFilterWidget->setFilterOptions({"Name", "Instructor"});
    leftLayout->addWidget(m_materialsFilterWidget);
    m_materialsTreeWidget = new QTreeWidget(this);
    m_materialsTreeWidget->setHeaderLabels({"Name", "Type", "ID", "Instructor"});
    m_materialsTreeWidget->setColumnHidden(2, true);
    m_materialsTreeWidget->setColumnHidden(3, true);
    leftLayout->addWidget(m_materialsTreeWidget);
    splitter->addWidget(leftWidget);

    // Right Panel
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
    splitter->setStretchFactor(1, 1);

    mainLayout->addWidget(splitter);

    // Connect signals
    connect(m_refreshButton, &QPushButton::clicked, this, &CourseListWidget::onRefresh);
    connect(m_materialsTreeWidget,
            &QTreeWidget::itemClicked,
            this,
            &CourseListWidget::onItemSelected);
    connect(m_startQuizButton, &QPushButton::clicked, this, &CourseListWidget::onStartQuiz);
    connect(m_submitQuizButton, &QPushButton::clicked, this, &CourseListWidget::onSubmitQuiz);
    connect(m_materialsFilterWidget,
            &FilterWidget::filterChanged,
            this,
            &CourseListWidget::applyFilter);
}

void CourseListWidget::onRefresh()
{
    m_materialsTreeWidget->clear();
    NetworkManager::instance().sendCommand("GET_ALL_CLASSES",
                                           QJsonObject(),
                                           [this](const QJsonObject &response) {
                                               handleClassesResponse(response);
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
        clearContentArea();
        m_contentGroup->setTitle("Select a material");
    } else {
        QMessageBox::critical(this, "Error", response["message"].toString());
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

void CourseListWidget::applyFilter()
{
    for (int i = 0; i < m_materialsTreeWidget->topLevelItemCount(); ++i) {
        applyFilterRecursive(m_materialsTreeWidget->topLevelItem(i),
                             m_materialsFilterWidget->filterText(),
                             m_materialsFilterWidget->currentFilterOption());
    }
}

bool CourseListWidget::applyFilterRecursive(QTreeWidgetItem *item,
                                            const QString &text,
                                            const QString &filterBy)
{
    bool anyChildMatches = false;
    for (int i = 0; i < item->childCount(); ++i) {
        if (applyFilterRecursive(item->child(i), text, filterBy)) {
            anyChildMatches = true;
        }
    }

    int column = (filterBy == "Name") ? 0 : 3;
    bool selfMatches = item->text(column).contains(text, Qt::CaseInsensitive);
    bool shouldBeVisible = selfMatches || anyChildMatches;
    item->setHidden(!shouldBeVisible);

    return shouldBeVisible;
}
