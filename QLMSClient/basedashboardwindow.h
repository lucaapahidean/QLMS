#ifndef BASEDASHBOARDWINDOW_H
#define BASEDASHBOARDWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
class QStackedWidget;
QT_END_NAMESPACE

class BaseDashboardWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit BaseDashboardWindow(const QString &username, int userId, QWidget *parent = nullptr);

signals:
    void logoutRequested();

protected:
    void setupMenuBar();
    void setCentralWorkspace(QWidget *widget);

    QString getUsername() const { return m_username; }
    int getUserId() const { return m_userId; }

protected slots:
    virtual void onLogout();

private:
    QString m_username;
    int m_userId;
    QStackedWidget *m_centralStack;
};

#endif // BASEDASHBOARDWINDOW_H
