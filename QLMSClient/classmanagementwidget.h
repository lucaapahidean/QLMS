#ifndef CLASSMANAGEMENTWIDGET_H
#define CLASSMANAGEMENTWIDGET_H

#include <QJsonArray>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QListWidget;
class QPushButton;
class QTableWidget;
QT_END_NAMESPACE

class ClassManagementWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ClassManagementWidget(QWidget *parent = nullptr);

private slots:
    void onRefreshClasses();
    void onAddClass();
    void onDeleteClass();
    void onClassSelected();
    void onAddCourse();
    void onDeleteCourse();
    void onAssignUser();
    void onRemoveUser();

private:
    void setupUi();
    void populateClasses(const QJsonArray &classes);
    void populateCourses(const QJsonArray &courses);
    void populateMembers(const QJsonArray &members);

    QListWidget *m_classesList;
    QListWidget *m_coursesList;
    QTableWidget *m_membersTable;
    QPushButton *m_refreshButton;
    QPushButton *m_addClassButton;
    QPushButton *m_deleteClassButton;
    QPushButton *m_addCourseButton;
    QPushButton *m_deleteCourseButton;
    QPushButton *m_assignUserButton;
    QPushButton *m_removeUserButton;

    QJsonArray m_classes;
    QJsonArray m_courses;
    QJsonArray m_members;
};

#endif // CLASSMANAGEMENTWIDGET_H
