#include "instructordashboard.h"
#include "coursemanagementwidget.h"
#include "gradingwidget.h"
#include "performancetrackingwidget.h"
#include <QTabWidget>

InstructorDashboard::InstructorDashboard(const QString &username, int userId, QWidget *parent)
    : BaseDashboardWindow(username, userId, parent)
{
    setWindowTitle(QString("QLMS Instructor - %1").arg(username));

    auto *tabWidget = new QTabWidget(this);

    auto *courseManagement = new CourseManagementWidget(this);
    tabWidget->addTab(courseManagement, "Course Management");

    auto *gradingWidget = new GradingWidget(this);
    tabWidget->addTab(gradingWidget, "Grading");

    auto *performanceWidget = new PerformanceTrackingWidget(userId, this);
    tabWidget->addTab(performanceWidget, "Track Performance");

    setCentralWorkspace(tabWidget);
}
