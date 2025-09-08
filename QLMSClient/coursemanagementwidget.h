#ifndef COURSEMANAGEMENTWIDGET_H
#define COURSEMANAGEMENTWIDGET_H

#include <QJsonArray>
#include <QJsonObject>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QTreeWidget;
class QTreeWidgetItem;
class QTextEdit;
class QPushButton;
class QGroupBox;
class QVBoxLayout;
QT_END_NAMESPACE

class CourseManagementWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CourseManagementWidget(QWidget *parent = nullptr);

private slots:
    void onRefresh();
    void onItemSelected();
    void onDeleteMaterial();
    void onAddMaterial();
    void handleClassesResponse(const QJsonObject &response);
    void handleCoursesResponse(const QJsonObject &response, QTreeWidgetItem *classItem);
    void handleMaterialsResponse(const QJsonObject &response, QTreeWidgetItem *courseItem);
    void handleMaterialDetailsResponse(const QJsonObject &response);
    void handleDeleteMaterialResponse(const QJsonObject &response);

private:
    void setupUi();
    void displayLesson(const QJsonObject &lesson);
    void displayQuiz(const QJsonObject &quiz);
    void clearContentArea();

    // UI Elements
    QTreeWidget *m_materialsTreeWidget;
    QPushButton *m_refreshButton;
    QPushButton *m_addButton;
    QPushButton *m_deleteButton;
    QGroupBox *m_contentGroup;
    QVBoxLayout *m_contentLayout;
    QTextEdit *m_contentView;
};

#endif // COURSEMANAGEMENTWIDGET_H
