#include "basedashboardwindow.h"
#include "networkmanager.h"
#include <QAction>
#include <QApplication>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QStackedWidget>
#include <QStatusBar>

BaseDashboardWindow::BaseDashboardWindow(const QString &username, int userId, QWidget *parent)
    : QMainWindow(parent)
    , m_username(username)
    , m_userId(userId)
{
    setWindowTitle(QString("QLMS - %1").arg(username));
    resize(1024, 768);

    m_centralStack = new QStackedWidget(this);
    setCentralWidget(m_centralStack);

    setupMenuBar();

    statusBar()->showMessage(QString("Logged in as: %1").arg(username));

    connect(&NetworkManager::instance(), &NetworkManager::disconnected, this, [this]() {
        QMessageBox::warning(this, "Disconnected", "Connection to server lost");
        close();
    });
}

void BaseDashboardWindow::setupMenuBar()
{
    QMenuBar *menuBar = this->menuBar();

    // File menu
    QMenu *fileMenu = menuBar->addMenu("&File");

    QAction *logoutAction = fileMenu->addAction("&Logout");
    logoutAction->setShortcut(QKeySequence("Ctrl+L"));
    connect(logoutAction, &QAction::triggered, this, &BaseDashboardWindow::onLogout);

    fileMenu->addSeparator();

    QAction *exitAction = fileMenu->addAction("E&xit");
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, qApp, &QApplication::quit);

    // Help menu
    QMenu *helpMenu = menuBar->addMenu("&Help");

    QAction *aboutAction = helpMenu->addAction("&About");
    connect(aboutAction, &QAction::triggered, this, [this]() {
        QMessageBox::about(this,
                           "About QLMS",
                           "Qt Learning Management System\n"
                           "Version 1.0.0\n\n"
                           "A secure client-server application for course management.");
    });
}

void BaseDashboardWindow::setCentralWorkspace(QWidget *widget)
{
    m_centralStack->addWidget(widget);
    m_centralStack->setCurrentWidget(widget);
}

void BaseDashboardWindow::onLogout()
{
    QJsonObject data;
    NetworkManager::instance().sendCommand("LOGOUT", data);
    emit logoutRequested();
}
