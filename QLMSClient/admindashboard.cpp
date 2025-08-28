#include "admindashboard.h"
#include "usermanagementwidget.h"

AdminDashboard::AdminDashboard(const QString &username, int userId, QWidget *parent)
    : BaseDashboardWindow(username, userId, parent)
{
    setWindowTitle(QString("QLMS Admin - %1").arg(username));

    auto *userManagementWidget = new UserManagementWidget(this);
    setCentralWorkspace(userManagementWidget);
}
