#include "instructordashboard.h"
#include "courseeditorwidget.h"
#include "gradingwidget.h"
#include <QTabWidget>

InstructorDashboard::InstructorDashboard(const QString &username, int userId, QWidget *parent)
    : BaseDashboardWindow(username, userId, parent)
{
    setWindowTitle(QString("QLMS Instructor - %1").arg(username));

    auto *tabWidget = new QTabWidget(this);

    auto *courseEditor = new CourseEditorWidget(this);
    tabWidget->addTab(courseEditor, "Course Editor");

    auto *gradingWidget = new GradingWidget(this);
    tabWidget->addTab(gradingWidget, "Grading");

    setCentralWorkspace(tabWidget);
}
