#include "studentdashboard.h"
#include "courselistwidget.h"
#include "quizhistorywidget.h"
#include <QTabWidget>

StudentDashboard::StudentDashboard(const QString &username, int userId, QWidget *parent)
    : BaseDashboardWindow(username, userId, parent)
{
    setWindowTitle(QString("QLMS Student - %1").arg(username));

    auto *tabWidget = new QTabWidget(this);

    auto *courseListWidget = new CourseListWidget(userId, this);
    tabWidget->addTab(courseListWidget, "Course Materials");

    auto *quizHistoryWidget = new QuizHistoryWidget(userId, this);
    tabWidget->addTab(quizHistoryWidget, "Quiz History");

    setCentralWorkspace(tabWidget);
}
