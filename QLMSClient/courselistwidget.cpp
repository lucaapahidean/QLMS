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
#include <QTextEdit>
#include <QVBoxLayout>

CourseListWidget::CourseListWidget(int studentId, QWidget *parent)
    : QWidget(parent)
    , m_studentId(studentId)
{
    setupUi();
    onRefreshMaterials();
}

void CourseListWidget::setupUi()
{
    auto *mainLayout = new QHBoxLayout(this);

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

    mainLayout->addWidget(splitter);

    // Connect signals
    connect(m_refreshButton, &QPushButton::clicked, this, &CourseListWidget::onRefreshMaterials);
    connect(m_materialsListWidget,
            &QListWidget::itemClicked,
            this,
            &CourseListWidget::onMaterialSelected);
    connect(m_startQuizButton, &QPushButton::clicked, this, &CourseListWidget::onStartQuiz);
    connect(m_submitQuizButton, &QPushButton::clicked, this, &CourseListWidget::onSubmitQuiz);
}

void CourseListWidget::onRefreshMaterials()
{
    NetworkManager::instance().sendCommand("GET_MATERIALS",
                                           QJsonObject(),
                                           [this](const QJsonObject &response) {
                                               handleMaterialsResponse(response);
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

    NetworkManager::instance()
        .sendCommand("FINISH_ATTEMPT", data, [this](const QJsonObject &response) {
            if (response["type"].toString() == "OK") {
                QMessageBox::information(this, "Success", "Quiz submitted successfully");
                clearContentArea();
                m_contentGroup->setTitle("Select a material");
            } else {
                QMessageBox::critical(this, "Error", response["message"].toString());
            }
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

void CourseListWidget::handleQuizDataResponse(const QJsonObject &response)
{
    if (response["type"].toString() == "DATA_RESPONSE") {
        m_currentQuiz = response["data"].toObject();
        displayQuiz(m_currentQuiz);
    } else {
        QMessageBox::critical(this, "Error", "Failed to load quiz");
    }
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
