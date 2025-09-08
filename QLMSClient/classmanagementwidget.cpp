#include "classmanagementwidget.h"
#include "networkmanager.h"
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>
#include <QTableWidget>
#include <QVBoxLayout>

ClassManagementWidget::ClassManagementWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
    onRefreshClasses();
}

void ClassManagementWidget::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    auto *headerLayout = new QHBoxLayout();
    auto *titleLabel = new QLabel("Class Management", this);
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

    auto *leftPanel = new QWidget(this);
    auto *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->addWidget(new QLabel("Classes", this));
    m_classesList = new QListWidget(this);
    leftLayout->addWidget(m_classesList);
    auto *classButtons = new QHBoxLayout();
    m_addClassButton = new QPushButton("Add Class", this);
    m_deleteClassButton = new QPushButton("Delete Class", this);
    classButtons->addWidget(m_addClassButton);
    classButtons->addWidget(m_deleteClassButton);
    leftLayout->addLayout(classButtons);
    splitter->addWidget(leftPanel);

    auto *middlePanel = new QWidget(this);
    auto *middleLayout = new QVBoxLayout(middlePanel);
    middleLayout->addWidget(new QLabel("Courses", this));
    m_coursesList = new QListWidget(this);
    middleLayout->addWidget(m_coursesList);
    auto *courseButtons = new QHBoxLayout();
    m_addCourseButton = new QPushButton("Add Course", this);
    m_deleteCourseButton = new QPushButton("Delete Course", this);
    courseButtons->addWidget(m_addCourseButton);
    courseButtons->addWidget(m_deleteCourseButton);
    middleLayout->addLayout(courseButtons);
    splitter->addWidget(middlePanel);

    auto *rightPanel = new QWidget(this);
    auto *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->addWidget(new QLabel("Members", this));
    m_membersTable = new QTableWidget(this);
    m_membersTable->setColumnCount(3);
    m_membersTable->setHorizontalHeaderLabels({"ID", "Username", "Role"});
    m_membersTable->horizontalHeader()->setStretchLastSection(true);
    rightLayout->addWidget(m_membersTable);
    auto *memberButtons = new QHBoxLayout();
    m_assignUserButton = new QPushButton("Assign User", this);
    m_removeUserButton = new QPushButton("Remove User", this);
    memberButtons->addWidget(m_assignUserButton);
    memberButtons->addWidget(m_removeUserButton);
    rightLayout->addLayout(memberButtons);
    splitter->addWidget(rightPanel);

    mainLayout->addWidget(splitter);

    connect(m_refreshButton, &QPushButton::clicked, this, &ClassManagementWidget::onRefreshClasses);
    connect(m_addClassButton, &QPushButton::clicked, this, &ClassManagementWidget::onAddClass);
    connect(m_deleteClassButton, &QPushButton::clicked, this, &ClassManagementWidget::onDeleteClass);
    connect(m_addCourseButton, &QPushButton::clicked, this, &ClassManagementWidget::onAddCourse);
    connect(m_deleteCourseButton,
            &QPushButton::clicked,
            this,
            &ClassManagementWidget::onDeleteCourse);
    connect(m_assignUserButton, &QPushButton::clicked, this, &ClassManagementWidget::onAssignUser);
    connect(m_removeUserButton, &QPushButton::clicked, this, &ClassManagementWidget::onRemoveUser);
    connect(m_classesList,
            &QListWidget::itemSelectionChanged,
            this,
            &ClassManagementWidget::onClassSelected);
}

void ClassManagementWidget::onRefreshClasses()
{
    NetworkManager::instance().sendCommand("GET_ALL_CLASSES",
                                           QJsonObject(),
                                           [this](const QJsonObject &response) {
                                               if (response["type"].toString() == "DATA_RESPONSE") {
                                                   populateClasses(response["data"].toArray());
                                               }
                                           });
}

void ClassManagementWidget::onAddClass()
{
    bool ok;
    QString className
        = QInputDialog::getText(this, "Add Class", "Class Name:", QLineEdit::Normal, "", &ok);
    if (ok && !className.isEmpty()) {
        QJsonObject data;
        data["class_name"] = className;
        NetworkManager::instance()
            .sendCommand("CREATE_CLASS", data, [this](const QJsonObject &response) {
                if (response["type"].toString() == "OK") {
                    onRefreshClasses();
                } else {
                    QMessageBox::critical(this, "Error", response["message"].toString());
                }
            });
    }
}

void ClassManagementWidget::onDeleteClass()
{
    int row = m_classesList->currentRow();
    if (row < 0)
        return;

    int classId = m_classes[row].toObject()["class_id"].toInt();
    QMessageBox::StandardButton reply = QMessageBox::question(this,
                                                              "Delete Class",
                                                              "Are you sure?",
                                                              QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        QJsonObject data;
        data["class_id"] = classId;
        NetworkManager::instance()
            .sendCommand("DELETE_CLASS", data, [this](const QJsonObject &response) {
                if (response["type"].toString() == "OK") {
                    onRefreshClasses();
                } else {
                    QMessageBox::critical(this, "Error", response["message"].toString());
                }
            });
    }
}

void ClassManagementWidget::onClassSelected()
{
    int row = m_classesList->currentRow();
    if (row < 0) {
        m_coursesList->clear();
        m_membersTable->setRowCount(0);
        return;
    }

    int classId = m_classes[row].toObject()["class_id"].toInt();
    QJsonObject data;
    data["class_id"] = classId;

    NetworkManager::instance().sendCommand("GET_COURSES_FOR_CLASS",
                                           data,
                                           [this](const QJsonObject &response) {
                                               if (response["type"].toString() == "DATA_RESPONSE") {
                                                   populateCourses(response["data"].toArray());
                                               }
                                           });

    NetworkManager::instance().sendCommand("GET_CLASS_MEMBERS",
                                           data,
                                           [this](const QJsonObject &response) {
                                               if (response["type"].toString() == "DATA_RESPONSE") {
                                                   populateMembers(response["data"].toArray());
                                               }
                                           });
}

void ClassManagementWidget::onAddCourse()
{
    int classRow = m_classesList->currentRow();
    if (classRow < 0)
        return;

    int classId = m_classes[classRow].toObject()["class_id"].toInt();
    bool ok;
    QString courseName
        = QInputDialog::getText(this, "Add Course", "Course Name:", QLineEdit::Normal, "", &ok);
    if (ok && !courseName.isEmpty()) {
        QJsonObject data;
        data["course_name"] = courseName;
        data["class_id"] = classId;
        NetworkManager::instance()
            .sendCommand("CREATE_COURSE", data, [this](const QJsonObject &response) {
                if (response["type"].toString() == "OK") {
                    onClassSelected();
                } else {
                    QMessageBox::critical(this, "Error", response["message"].toString());
                }
            });
    }
}

void ClassManagementWidget::onDeleteCourse()
{
    int courseRow = m_coursesList->currentRow();
    if (courseRow < 0)
        return;

    int courseId = m_courses[courseRow].toObject()["course_id"].toInt();
    QMessageBox::StandardButton reply = QMessageBox::question(this,
                                                              "Delete Course",
                                                              "Are you sure?",
                                                              QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        QJsonObject data;
        data["course_id"] = courseId;
        NetworkManager::instance()
            .sendCommand("DELETE_COURSE", data, [this](const QJsonObject &response) {
                if (response["type"].toString() == "OK") {
                    onClassSelected();
                } else {
                    QMessageBox::critical(this, "Error", response["message"].toString());
                }
            });
    }
}

void ClassManagementWidget::onAssignUser()
{
    int classRow = m_classesList->currentRow();
    if (classRow < 0)
        return;

    int classId = m_classes[classRow].toObject()["class_id"].toInt();
    bool ok;
    int userId = QInputDialog::getInt(this, "Assign User", "User ID:", 0, 0, 100000, 1, &ok);
    if (ok) {
        QJsonObject data;
        data["user_id"] = userId;
        data["class_id"] = classId;
        NetworkManager::instance()
            .sendCommand("ASSIGN_USER_TO_CLASS", data, [this](const QJsonObject &response) {
                if (response["type"].toString() == "OK") {
                    onClassSelected();
                } else {
                    QMessageBox::critical(this, "Error", response["message"].toString());
                }
            });
    }
}

void ClassManagementWidget::onRemoveUser()
{
    int memberRow = m_membersTable->currentRow();
    if (memberRow < 0)
        return;
    int classRow = m_classesList->currentRow();
    if (classRow < 0)
        return;

    int userId = m_members[memberRow].toObject()["user_id"].toInt();
    int classId = m_classes[classRow].toObject()["class_id"].toInt();

    QMessageBox::StandardButton reply = QMessageBox::question(this,
                                                              "Remove User",
                                                              "Are you sure?",
                                                              QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        QJsonObject data;
        data["user_id"] = userId;
        data["class_id"] = classId;
        NetworkManager::instance()
            .sendCommand("REMOVE_USER_FROM_CLASS", data, [this](const QJsonObject &response) {
                if (response["type"].toString() == "OK") {
                    onClassSelected();
                } else {
                    QMessageBox::critical(this, "Error", response["message"].toString());
                }
            });
    }
}

void ClassManagementWidget::populateClasses(const QJsonArray &classes)
{
    m_classes = classes;
    m_classesList->clear();
    for (const auto &val : m_classes) {
        m_classesList->addItem(val.toObject()["class_name"].toString());
    }
}

void ClassManagementWidget::populateCourses(const QJsonArray &courses)
{
    m_courses = courses;
    m_coursesList->clear();
    for (const auto &val : m_courses) {
        m_coursesList->addItem(val.toObject()["course_name"].toString());
    }
}

void ClassManagementWidget::populateMembers(const QJsonArray &members)
{
    m_members = members;
    m_membersTable->setRowCount(0);
    for (const auto &val : m_members) {
        QJsonObject member = val.toObject();
        int row = m_membersTable->rowCount();
        m_membersTable->insertRow(row);
        m_membersTable->setItem(row,
                                0,
                                new QTableWidgetItem(QString::number(member["user_id"].toInt())));
        m_membersTable->setItem(row, 1, new QTableWidgetItem(member["username"].toString()));
        m_membersTable->setItem(row, 2, new QTableWidgetItem(member["role"].toString()));
    }
}
