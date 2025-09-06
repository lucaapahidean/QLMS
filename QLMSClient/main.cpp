#include "admindashboard.h"
#include "instructordashboard.h"
#include "logindialog.h"
#include "networkmanager.h"
#include "studentdashboard.h"
#include <QApplication>

BaseDashboardWindow *createDashboard(const QString &role, const QString &username, int userId)
{
    if (role == "admin") {
        return new AdminDashboard(username, userId);
    } else if (role == "instructor") {
        return new InstructorDashboard(username, userId);
    } else if (role == "student") {
        return new StudentDashboard(username, userId);
    }
    return nullptr;
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("QLMS Client");
    app.setApplicationDisplayName("Qt Learning Management System");
    app.setOrganizationName("QLMS");

    BaseDashboardWindow *currentDashboard = nullptr;

    // Main application loop - keeps running until user explicitly exits
    while (true) {
        // Clean up previous dashboard if it exists
        if (currentDashboard) {
            currentDashboard->deleteLater();
            currentDashboard = nullptr;
        }

        // Show login dialog
        LoginDialog loginDialog;
        if (loginDialog.exec() != QDialog::Accepted) {
            // User cancelled login or closed dialog - exit application
            break;
        }

        // Get user information
        QString username = loginDialog.getUsername();
        QString role = loginDialog.getRole();
        int userId = loginDialog.getUserId();

        // Create appropriate dashboard based on role
        currentDashboard = createDashboard(role, username, userId);
        if (!currentDashboard) {
            // Should not happen, but handle gracefully
            break;
        }

        // Connect logout signal to continue the loop
        bool logoutRequested = false;
        QObject::connect(currentDashboard,
                         &BaseDashboardWindow::logoutRequested,
                         [&logoutRequested]() {
                             logoutRequested = true;
                             QApplication::quit(); // Exit the current event loop
                         });

        // Show dashboard and start event loop
        currentDashboard->show();
        app.exec();

        // Check if we exited due to logout or application exit
        if (!logoutRequested) {
            // Application was closed normally (Exit menu or window close)
            break;
        }
        // If logoutRequested is true, continue the loop to show login again
    }

    // Final cleanup
    if (currentDashboard) {
        delete currentDashboard;
    }
    NetworkManager::instance().disconnectFromServer();

    return 0;
}
