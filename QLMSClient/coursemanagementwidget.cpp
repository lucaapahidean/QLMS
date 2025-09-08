#include "coursemanagementwidget.h"
#include "materialcreationwizard.h"
#include "networkmanager.h"
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>
#include <QTextEdit>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

CourseManagementWidget::CourseManagementWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
    onRefresh();
}

void CourseManagementWidget::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);

    // Header
    auto *headerLayout = new QHBoxLayout();
    auto *titleLabel = new QLabel("Course Management", this);
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
    leftLayout->addWidget(new QLabel("Course Materials", this));
    m_materialsTreeWidget = new QTreeWidget(this);
    m_materialsTreeWidget->setColumnWidth(0, 350);
    m_materialsTreeWidget->setHeaderLabels({"Name", "Type", "ID"});
    m_materialsTreeWidget->setColumnHidden(2, true); // Hide ID column
    leftLayout->addWidget(m_materialsTreeWidget);
    auto *buttonLayout = new QHBoxLayout();
    m_addButton = new QPushButton("Add Material...", this);
    m_deleteButton = new QPushButton("Delete Selected", this);
    m_deleteButton->setEnabled(false);
    buttonLayout->addWidget(m_addButton);
    buttonLayout->addWidget(m_deleteButton);
    leftLayout->addLayout(buttonLayout);
    splitter->addWidget(leftWidget);

    // Right panel
    auto *rightWidget = new QWidget(this);
    auto *rightLayout = new QVBoxLayout(rightWidget);
    m_contentGroup = new QGroupBox("Select a material to view its content", this);
    m_contentLayout = new QVBoxLayout(m_contentGroup);
    m_contentView = new QTextEdit(this);
    m_contentView->setReadOnly(true);
    m_contentLayout->addWidget(m_contentView);
    rightLayout->addWidget(m_contentGroup);
    splitter->addWidget(rightWidget);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);

    mainLayout->addWidget(splitter);

    // Connect signals
    connect(m_refreshButton, &QPushButton::clicked, this, &CourseManagementWidget::onRefresh);
    connect(m_addButton, &QPushButton::clicked, this, &CourseManagementWidget::onAddMaterial);
    connect(m_deleteButton, &QPushButton::clicked, this, &CourseManagementWidget::onDeleteMaterial);
    connect(m_materialsTreeWidget,
            &QTreeWidget::itemSelectionChanged,
            this,
            &CourseManagementWidget::onItemSelected);
}

void CourseManagementWidget::onRefresh()
{
    // Disconnect the signal to prevent onItemSelected from being called during the refresh
    disconnect(m_materialsTreeWidget,
               &QTreeWidget::itemSelectionChanged,
               this,
               &CourseManagementWidget::onItemSelected);

    m_materialsTreeWidget->clear();
    NetworkManager::instance().sendCommand("GET_ALL_CLASSES",
                                           QJsonObject(),
                                           [this](const QJsonObject &response) {
                                               handleClassesResponse(response);
                                               // Reconnect the signal now that the tree is populated
                                               connect(m_materialsTreeWidget,
                                                       &QTreeWidget::itemSelectionChanged,
                                                       this,
                                                       &CourseManagementWidget::onItemSelected);
                                           });
}

void CourseManagementWidget::onItemSelected()
{
    QTreeWidgetItem *item = m_materialsTreeWidget->currentItem();
    if (!item) {
        m_deleteButton->setEnabled(false);
        return;
    }

    QString type = item->text(1);
    m_deleteButton->setEnabled(type == "lesson" || type == "quiz");

    if (type == "lesson" || type == "quiz") {
        int materialId = item->text(2).toInt();
        QJsonObject data;
        data["material_id"] = materialId;
        NetworkManager::instance().sendCommand("GET_MATERIAL_DETAILS",
                                               data,
                                               [this](const QJsonObject &response) {
                                                   handleMaterialDetailsResponse(response);
                                               });
    } else {
        clearContentArea();
    }
}

void CourseManagementWidget::onDeleteMaterial()
{
    QTreeWidgetItem *item = m_materialsTreeWidget->currentItem();
    if (!item || (item->text(1) != "lesson" && item->text(1) != "quiz"))
        return;

    int materialId = item->text(2).toInt();
    QString materialTitle = item->text(0);

    int ret = QMessageBox::question(
        this,
        "Delete Material",
        QString("Are you sure you want to delete '%1'? This action cannot be undone.")
            .arg(materialTitle),
        QMessageBox::Yes | QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        QJsonObject data;
        data["material_id"] = materialId;

        NetworkManager::instance().sendCommand("DELETE_MATERIAL",
                                               data,
                                               [this](const QJsonObject &response) {
                                                   handleDeleteMaterialResponse(response);
                                               });
    }
}

void CourseManagementWidget::onAddMaterial()
{
    auto *wizard = new MaterialCreationWizard(this);
    if (wizard->exec() == QDialog::Accepted) {
        onRefresh();
    }
}

void CourseManagementWidget::handleClassesResponse(const QJsonObject &response)
{
    if (response["type"].toString() == "DATA_RESPONSE") {
        QJsonArray classes = response["data"].toArray();
        for (const auto &val : classes) {
            QJsonObject classObj = val.toObject();
            auto *classItem = new QTreeWidgetItem(m_materialsTreeWidget);
            classItem->setText(0, classObj["class_name"].toString());
            classItem->setText(1, "class");
            classItem->setText(2, QString::number(classObj["class_id"].toInt()));

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

void CourseManagementWidget::handleCoursesResponse(const QJsonObject &response,
                                                   QTreeWidgetItem *classItem)
{
    if (response["type"].toString() == "DATA_RESPONSE") {
        QJsonArray courses = response["data"].toArray();
        for (const auto &val : courses) {
            QJsonObject courseObj = val.toObject();
            auto *courseItem = new QTreeWidgetItem(classItem);
            courseItem->setText(0, courseObj["course_name"].toString());
            courseItem->setText(1, "course");
            courseItem->setText(2, QString::number(courseObj["course_id"].toInt()));

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

void CourseManagementWidget::handleMaterialsResponse(const QJsonObject &response,
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
        }
    }
}

void CourseManagementWidget::handleMaterialDetailsResponse(const QJsonObject &response)
{
    if (response["type"].toString() == "DATA_RESPONSE") {
        QJsonObject material = response["data"].toObject();
        QString type = material["type"].toString();

        if (type == "lesson") {
            displayLesson(material);
        } else if (type == "quiz") {
            displayQuiz(material);
        }
    } else {
        QMessageBox::critical(this,
                              "Error",
                              response["message"].toString("Failed to load material details"));
    }
}

void CourseManagementWidget::handleDeleteMaterialResponse(const QJsonObject &response)
{
    if (response["type"].toString() == "OK") {
        QMessageBox::information(this, "Success", "Material deleted successfully.");
        clearContentArea();
        onRefresh();
    } else {
        QMessageBox::critical(this,
                              "Error",
                              response["message"].toString("Failed to delete material."));
    }
}

void CourseManagementWidget::displayLesson(const QJsonObject &lesson)
{
    m_contentGroup->setTitle(lesson["title"].toString());
    m_contentView->setPlainText(lesson["content"].toString());
}

void CourseManagementWidget::displayQuiz(const QJsonObject &quiz)
{
    m_contentGroup->setTitle(quiz["title"].toString());

    QString quizDetails;
    quizDetails += "Title: " + quiz["title"].toString() + "\n";
    quizDetails += "Max Attempts: " + QString::number(quiz["max_attempts"].toInt()) + "\n";
    quizDetails += "Feedback Type: " + quiz["feedback_type"].toString() + "\n\n";
    quizDetails += "--- Questions ---\n\n";

    QJsonArray questions = quiz["questions"].toArray();
    for (int i = 0; i < questions.size(); ++i) {
        QJsonObject question = questions[i].toObject();
        quizDetails += QString("Q%1: %2\n").arg(i + 1).arg(question["prompt"].toString());

        QString type = question["question_type"].toString();
        if (type == "radio" || type == "checkbox") {
            QJsonArray options = question["options"].toArray();
            for (int j = 0; j < options.size(); ++j) {
                QJsonObject option = options[j].toObject();
                QString correctness = option["is_correct"].toBool() ? " (Correct)" : "";
                quizDetails += QString("  - %1%2\n").arg(option["text"].toString()).arg(correctness);
            }
        }
        quizDetails += "\n";
    }

    m_contentView->setPlainText(quizDetails);
}

void CourseManagementWidget::clearContentArea()
{
    m_contentGroup->setTitle("Select a material to view its content");
    m_contentView->clear();
}
