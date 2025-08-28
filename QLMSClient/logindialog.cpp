#include "logindialog.h"
#include "networkmanager.h"
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent)
    , m_userId(0)
{
    setupUi();
    setWindowTitle("QLMS - Login");
    setFixedSize(400, 300);

    connect(&NetworkManager::instance(), &NetworkManager::connected, this, [this]() {
        setConnectionState(true);
    });
    connect(&NetworkManager::instance(), &NetworkManager::disconnected, this, [this]() {
        setConnectionState(false);
    });
    connect(&NetworkManager::instance(),
            &NetworkManager::errorOccurred,
            this,
            [this](const QString &error) {
                QMessageBox::critical(this, "Connection Error", error);
                setConnectionState(false);
            });
}

void LoginDialog::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);

    // Server connection group
    auto *connectionGroup = new QGroupBox("Server Connection", this);
    auto *connectionLayout = new QGridLayout(connectionGroup);

    connectionLayout->addWidget(new QLabel("Server:", this), 0, 0);
    m_serverEdit = new QLineEdit("localhost", this);
    connectionLayout->addWidget(m_serverEdit, 0, 1);

    connectionLayout->addWidget(new QLabel("Port:", this), 1, 0);
    m_portEdit = new QLineEdit("8080", this);
    connectionLayout->addWidget(m_portEdit, 1, 1);

    m_connectButton = new QPushButton("Connect", this);
    connectionLayout->addWidget(m_connectButton, 2, 0, 1, 2);

    mainLayout->addWidget(connectionGroup);

    // Login group
    auto *loginGroup = new QGroupBox("Login", this);
    auto *loginLayout = new QGridLayout(loginGroup);

    loginLayout->addWidget(new QLabel("Username:", this), 0, 0);
    m_usernameEdit = new QLineEdit(this);
    loginLayout->addWidget(m_usernameEdit, 0, 1);

    loginLayout->addWidget(new QLabel("Password:", this), 1, 0);
    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    loginLayout->addWidget(m_passwordEdit, 1, 1);

    m_loginButton = new QPushButton("Login", this);
    m_loginButton->setEnabled(false);
    loginLayout->addWidget(m_loginButton, 2, 0, 1, 2);

    mainLayout->addWidget(loginGroup);

    // Status label
    m_statusLabel = new QLabel("Not connected", this);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_statusLabel);

    mainLayout->addStretch();

    // Connect signals
    connect(m_connectButton, &QPushButton::clicked, this, &LoginDialog::onConnectClicked);
    connect(m_loginButton, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);
    connect(m_passwordEdit, &QLineEdit::returnPressed, this, &LoginDialog::onLoginClicked);
}

void LoginDialog::setConnectionState(bool connected)
{
    m_serverEdit->setEnabled(!connected);
    m_portEdit->setEnabled(!connected);
    m_connectButton->setText(connected ? "Disconnect" : "Connect");
    m_loginButton->setEnabled(connected);
    m_statusLabel->setText(connected ? "Connected to server" : "Not connected");
}

void LoginDialog::onConnectClicked()
{
    if (NetworkManager::instance().isConnected()) {
        NetworkManager::instance().disconnectFromServer();
    } else {
        QString host = m_serverEdit->text();
        quint16 port = m_portEdit->text().toUShort();

        m_statusLabel->setText("Connecting...");
        if (!NetworkManager::instance().connectToServer(host, port)) {
            QMessageBox::critical(this, "Connection Error", "Failed to connect to server");
            m_statusLabel->setText("Not connected");
        }
    }
}

void LoginDialog::onLoginClicked()
{
    if (m_usernameEdit->text().isEmpty() || m_passwordEdit->text().isEmpty()) {
        QMessageBox::warning(this, "Login", "Please enter username and password");
        return;
    }

    QJsonObject data;
    data["username"] = m_usernameEdit->text();
    data["password"] = m_passwordEdit->text();

    m_loginButton->setEnabled(false);
    m_statusLabel->setText("Authenticating...");

    NetworkManager::instance().sendCommand("LOGIN", data, [this](const QJsonObject &response) {
        onLoginResponse(response);
    });
}

void LoginDialog::onLoginResponse(const QJsonObject &response)
{
    m_loginButton->setEnabled(true);

    if (response["type"].toString() == "LOGIN_SUCCESS") {
        QJsonObject user = response["user"].toObject();
        m_username = user["username"].toString();
        m_role = user["role"].toString();
        m_userId = user["user_id"].toInt();
        accept();
    } else {
        m_statusLabel->setText("Login failed");
        QMessageBox::warning(this,
                             "Login Failed",
                             response["message"].toString("Invalid credentials"));
        m_passwordEdit->clear();
        m_passwordEdit->setFocus();
    }
}

QString LoginDialog::getUsername() const
{
    return m_username;
}

QString LoginDialog::getRole() const
{
    return m_role;
}

int LoginDialog::getUserId() const
{
    return m_userId;
}
