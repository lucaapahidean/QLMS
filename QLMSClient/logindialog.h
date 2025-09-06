#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QJsonObject>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QPushButton;
class QLabel;
QT_END_NAMESPACE

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);

    QString getUsername() const;
    QString getRole() const;
    int getUserId() const;

private slots:
    void onLoginClicked();
    void onLoginResponse(const QJsonObject &response);
    void onConnectClicked();

private:
    void setupUi();
    void setConnectionState(bool connected);
    void updateConnectionState(); // New method to check current state

    QLineEdit *m_serverEdit;
    QLineEdit *m_portEdit;
    QPushButton *m_connectButton;
    QLineEdit *m_usernameEdit;
    QLineEdit *m_passwordEdit;
    QPushButton *m_loginButton;
    QLabel *m_statusLabel;

    QString m_username;
    QString m_role;
    int m_userId;
};

#endif // LOGINDIALOG_H
