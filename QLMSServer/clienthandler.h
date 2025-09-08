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
    void handleGetAllClasses();
    void handleCreateClass(const QJsonObject &data);
    void handleDeleteClass(const QJsonObject &data);
    void handleAssignUserToClass(const QJsonObject &data);
    void handleRemoveUserFromClass(const QJsonObject &data);
    void handleGetClassMembers(const QJsonObject &data);
    void handleGetCoursesForClass(const QJsonObject &data);
    void handleCreateCourse(const QJsonObject &data);
    void handleDeleteCourse(const QJsonObject &data);
    void handleGetMaterialsForCourse(const QJsonObject &data);
    void handleGetMaterialDetails(const QJsonObject &data);
    void handleDeleteMaterial(const QJsonObject &data);
    void handleCreateLesson(const QJsonObject &data);
    void handleCreateQuizWithQuestions(const QJsonObject &data);
    void handleStartQuiz(const QJsonObject &data);
    void handleFinishAttempt(const QJsonObject &data);
    void handleGetPendingAttempts();
    void handleGetMyAttempts();
    void handleGetAttemptDetails(const QJsonObject &data);
    void handleGetStudentAttemptsForQuiz(const QJsonObject &data);
    void handleSubmitGrade(const QJsonObject &data);
    void handleGetClassStatistics(const QJsonObject &data);
    void handleGetCourseStatistics(const QJsonObject &data);

    QSslSocket *m_socket;
    QByteArray m_buffer;
    std::shared_ptr<User> m_currentUser;
};

#endif // CLIENTHANDLER_H
