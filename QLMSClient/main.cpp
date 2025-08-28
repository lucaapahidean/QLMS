#include "admindashboard.h"
#include "instructordashboard.h"
#include "logindialog.h"
#include "networkmanager.h"
#include "studentdashboard.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("QLMS Client");
    app.setApplicationDisplayName("Qt Learning Management System");
    app.setOrganizationName("QLMS");

    // Show login dialog
    LoginDialog loginDialog;
    if (loginDialog.exec() != QDialog::Accepted) {
        return 0;
    }

    // Get user information
    QString username = loginDialog.getUsername();
    QString role = loginDialog.getRole();
    int userId = loginDialog.getUserId();

    // Create and show appropriate dashboard based on role
    BaseDashboardWindow *dashboard = nullptr;

    if (role == "admin") {
        dashboard = new AdminDashboard(username, userId);
    } else if (role == "instructor") {
        dashboard = new InstructorDashboard(username, userId);
    } else if (role == "student") {
        dashboard = new StudentDashboard(username, userId);
    } else {
        // Should not happen
        return 1;
    }

    dashboard->show();

    int result = app.exec();

    // Cleanup
    delete dashboard;
    NetworkManager::instance().disconnectFromServer();

    return result;
}
