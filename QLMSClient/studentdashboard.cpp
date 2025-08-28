#include "studentdashboard.h"
#include "courselistwidget.h"

StudentDashboard::StudentDashboard(const QString &username, int userId, QWidget *parent)
    : BaseDashboardWindow(username, userId, parent)
{
    setWindowTitle(QString("QLMS Student - %1").arg(username));

    auto *courseListWidget = new CourseListWidget(userId, this);
    setCentralWorkspace(courseListWidget);
}
