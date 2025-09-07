#ifndef CLIENTHANDLER_H
#define CLIENTHANDLER_H

#include <memory>
#include <QJsonObject>
#include <QObject>
#include <QSslSocket>

class User;

class ClientHandler : public QObject
{
    Q_OBJECT

public:
    explicit ClientHandler(QSslSocket *socket, QObject *parent = nullptr);
    ~ClientHandler();

signals:
    void logMessage(const QString &message);
    void clientDisconnected(ClientHandler *handler);

public slots:
    void startProcessing();

private slots:
    void onReadyRead();
    void onDisconnected();
    void onSslErrors(const QList<QSslError> &errors);

private:
    void processMessage(const QJsonObject &message);
    void sendResponse(const QJsonObject &response);

    // Command handlers
    void handleLogin(const QJsonObject &data);
    void handleLogout();
    void handleCreateUser(const QJsonObject &data);
    void handleDeleteUser(const QJsonObject &data);
    void handleGetAllUsers();
    void handleGetMaterials();
    void handleCreateQuiz(const QJsonObject &data);
    void handleAddQuestion(const QJsonObject &data);
    void handleStartQuiz(const QJsonObject &data);
    void handleFinishAttempt(const QJsonObject &data);
    void handleGetPendingAttempts();
    void handleSubmitGrade(const QJsonObject &data);

    // New handlers for quiz history
    void handleGetMyAttempts();
    void handleGetAttemptDetails(const QJsonObject &data);

    QSslSocket *m_socket;
    QByteArray m_buffer;
    std::shared_ptr<User> m_currentUser;
};

#endif // CLIENTHANDLER_H
