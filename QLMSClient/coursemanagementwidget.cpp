#include "coursemanagementwidget.h"
#include "materialcreationwizard.h"
#include "networkmanager.h"
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>
#include <QTextEdit>
#include <QVBoxLayout>

CourseManagementWidget::CourseManagementWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
    onRefreshMaterials();
}

void CourseManagementWidget::setupUi()
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

    auto *buttonLayout = new QHBoxLayout();
    m_refreshButton = new QPushButton("Refresh", this);
    m_addButton = new QPushButton("Add Material...", this);
    m_deleteButton = new QPushButton("Delete Selected", this);
    m_deleteButton->setEnabled(false);
    buttonLayout->addWidget(m_refreshButton);
    buttonLayout->addWidget(m_addButton);
    buttonLayout->addWidget(m_deleteButton);
    leftLayout->addLayout(buttonLayout);

    splitter->addWidget(leftWidget);

    // Right panel - Content area
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
    splitter->setStretchFactor(1, 2);

    mainLayout->addWidget(splitter);

    // Connect signals
    connect(m_refreshButton,
            &QPushButton::clicked,
            this,
            &CourseManagementWidget::onRefreshMaterials);
    connect(m_addButton, &QPushButton::clicked, this, &CourseManagementWidget::onAddMaterial);
    connect(m_deleteButton, &QPushButton::clicked, this, &CourseManagementWidget::onDeleteMaterial);
    connect(m_materialsListWidget, &QListWidget::itemSelectionChanged, this, [this]() {
        m_deleteButton->setEnabled(m_materialsListWidget->currentRow() >= 0);
    });
    connect(m_materialsListWidget,
            &QListWidget::itemClicked,
            this,
            &CourseManagementWidget::onMaterialSelected);
}

void CourseManagementWidget::onRefreshMaterials()
{
    NetworkManager::instance().sendCommand("GET_MATERIALS",
                                           QJsonObject(),
                                           [this](const QJsonObject &response) {
                                               handleMaterialsResponse(response);
                                           });
}

void CourseManagementWidget::onMaterialSelected()
{
    int row = m_materialsListWidget->currentRow();
    if (row < 0 || row >= m_materials.size())
        return;

    QJsonObject material = m_materials[row].toObject();
    QString type = material["type"].toString();

    if (type == "lesson") {
        // For lessons, the content is already available from the GET_MATERIALS call.
        // No need to make another network request.
        displayLesson(material);
    } else if (type == "quiz") {
        // For quizzes, we fetch details to get correct answers for the instructor view.
        int materialId = material["material_id"].toInt();
        QJsonObject data;
        data["material_id"] = materialId;
        NetworkManager::instance().sendCommand("GET_MATERIAL_DETAILS",
                                               data,
                                               [this](const QJsonObject &response) {
                                                   handleMaterialDetailsResponse(response);
                                               });
    }
}

void CourseManagementWidget::onDeleteMaterial()
{
    int row = m_materialsListWidget->currentRow();
    if (row < 0)
        return;

    int materialId = m_materials[row].toObject()["material_id"].toInt();
    QString materialTitle = m_materials[row].toObject()["title"].toString();

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
        onRefreshMaterials();
    }
}

void CourseManagementWidget::handleMaterialsResponse(const QJsonObject &response)
{
    if (response["type"].toString() == "DATA_RESPONSE") {
        m_materials = response["data"].toArray();
        populateMaterialsList(m_materials);
        clearContentArea();
    } else {
        QMessageBox::critical(this, "Error", "Failed to fetch materials");
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
        onRefreshMaterials();
    } else {
        QMessageBox::critical(this,
                              "Error",
                              response["message"].toString("Failed to delete material."));
    }
}

void CourseManagementWidget::populateMaterialsList(const QJsonArray &materials)
{
    m_materialsListWidget->clear();
    for (const QJsonValue &value : materials) {
        QJsonObject material = value.toObject();
        QString text
            = QString("[%1] %2").arg(material["type"].toString()).arg(material["title"].toString());
        m_materialsListWidget->addItem(text);
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
