#include "admindashboard.h"
#include "classmanagementwidget.h"
#include "usermanagementwidget.h"
#include <QTabWidget>

AdminDashboard::AdminDashboard(const QString &username, int userId, QWidget *parent)
    : BaseDashboardWindow(username, userId, parent)
{
    setWindowTitle(QString("QLMS Admin - %1").arg(username));

    auto *tabWidget = new QTabWidget(this);

    auto *userManagementWidget = new UserManagementWidget(this);
    tabWidget->addTab(userManagementWidget, "User Management");

    auto *classManagementWidget = new ClassManagementWidget(this);
    tabWidget->addTab(classManagementWidget, "Class Management");

    setCentralWorkspace(tabWidget);
}
