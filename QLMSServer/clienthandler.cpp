#include "clienthandler.h"
#include "coursematerial.h"
#include "databasemanager.h"
#include "user.h"
#include <QCryptographicHash>
#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSslCertificate>
#include <QSslKey>
#include <QSslSocket>
#include <QThread>

ClientHandler::ClientHandler(QSslSocket *socket, QObject *parent)
    : QObject(parent)
    , m_socket(socket)
{}

ClientHandler::~ClientHandler() {}

void ClientHandler::startProcessing()
{
    // Now that we are in the correct thread, connect the socket's signals.
    connect(m_socket, &QSslSocket::readyRead, this, &ClientHandler::onReadyRead);
    connect(m_socket, &QSslSocket::disconnected, this, &ClientHandler::onDisconnected);
    connect(m_socket, &QSslSocket::sslErrors, this, &ClientHandler::onSslErrors);

    emit logMessage(QString("Client connected from %1").arg(m_socket->peerAddress().toString()));
}

void ClientHandler::onReadyRead()
{
    if (!m_socket)
        return;

    m_buffer.append(m_socket->readAll());

    int pos;
    while ((pos = m_buffer.indexOf('\n')) != -1) {
        QByteArray messageData = m_buffer.left(pos);
        m_buffer.remove(0, pos + 1);

        QJsonDocument doc = QJsonDocument::fromJson(messageData);
        if (!doc.isNull() && doc.isObject()) {
            processMessage(doc.object());
        }
    }
}

void ClientHandler::onDisconnected()
{
    emit logMessage(QString("Client disconnected from %1")
                        .arg(m_socket ? m_socket->peerAddress().toString() : "unknown"));
    emit clientDisconnected(this);

    // The worker's job is done, so we tell its thread to quit the event loop.
    if (thread()) {
        thread()->quit();
    }
}

void ClientHandler::onSslErrors(const QList<QSslError> &errors)
{
    for (const QSslError &error : errors) {
        emit logMessage(QString("SSL Error: %1").arg(error.errorString()));
    }
    m_socket->ignoreSslErrors();
}

void ClientHandler::processMessage(const QJsonObject &message)
{
    QString command = message["command"].toString();
    QJsonObject data = message["data"].toObject();

    emit logMessage(QString("Received command: %1").arg(command));

    if (command == "LOGIN") {
        handleLogin(data);
    } else if (command == "LOGOUT") {
        handleLogout();
    } else if (command == "GET_MATERIALS") {
        handleGetMaterials();
    } else if (!m_currentUser) {
        QJsonObject response;
        response["type"] = "ERROR";
        response["message"] = "Not authenticated";
        sendResponse(response);
    } else if (command == "CREATE_USER") {
        handleCreateUser(data);
    } else if (command == "DELETE_USER") {
        handleDeleteUser(data);
    } else if (command == "GET_ALL_USERS") {
        handleGetAllUsers();
    } else if (command == "CREATE_QUIZ") {
        handleCreateQuiz(data);
    } else if (command == "ADD_QUESTION") {
        handleAddQuestion(data);
    } else if (command == "START_QUIZ") {
        handleStartQuiz(data);
    } else if (command == "FINISH_ATTEMPT") {
        handleFinishAttempt(data);
    } else if (command == "GET_PENDING_ATTEMPTS") {
        handleGetPendingAttempts();
    } else if (command == "SUBMIT_GRADE") {
        handleSubmitGrade(data);
    } else if (command == "GET_MY_ATTEMPTS") {
        handleGetMyAttempts();
    } else if (command == "GET_ATTEMPT_DETAILS") {
        handleGetAttemptDetails(data);
    } else {
        QJsonObject response;
        response["type"] = "ERROR";
        response["message"] = "Unknown command";
        sendResponse(response);
    }
}

void ClientHandler::sendResponse(const QJsonObject &response)
{
    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState)
        return;

    QJsonDocument doc(response);
    QByteArray data = doc.toJson(QJsonDocument::Compact) + "\n";
    m_socket->write(data);
    m_socket->flush();
}

void ClientHandler::handleLogin(const QJsonObject &data)
{
    QString username = data["username"].toString();
    QString password = data["password"].toString();

    // Hash the password (in production, use proper salting and stronger hashing)
    QByteArray hash = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256);
    QString passwordHash = QString(hash.toHex());

    m_currentUser = DatabaseManager::instance().authenticateUser(username, passwordHash);

    QJsonObject response;
    if (m_currentUser) {
        response["type"] = "LOGIN_SUCCESS";
        response["user"] = m_currentUser->toJson();
        emit logMessage(QString("User %1 logged in successfully").arg(username));
    } else {
        response["type"] = "LOGIN_FAIL";
        response["message"] = "Invalid username or password";
        emit logMessage(QString("Failed login attempt for user %1").arg(username));
    }

    sendResponse(response);
}

void ClientHandler::handleLogout()
{
    if (m_currentUser) {
        emit logMessage(QString("User %1 logged out").arg(m_currentUser->getUsername()));
        m_currentUser.reset();
    }

    QJsonObject response;
    response["type"] = "OK";
    response["message"] = "Logged out successfully";
    sendResponse(response);
}

void ClientHandler::handleCreateUser(const QJsonObject &data)
{
    if (!m_currentUser || m_currentUser->getRole() != "admin") {
        QJsonObject response;
        response["type"] = "ERROR";
        response["message"] = "Unauthorized";
        sendResponse(response);
        return;
    }

    QString username = data["username"].toString();
    QString password = data["password"].toString();
    QString role = data["role"].toString();

    QByteArray hash = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256);
    QString passwordHash = QString(hash.toHex());

    bool success = DatabaseManager::instance().createUser(username, passwordHash, role);

    QJsonObject response;
    if (success) {
        response["type"] = "OK";
        response["message"] = "User created successfully";
    } else {
        response["type"] = "ERROR";
        response["message"] = "Failed to create user";
    }
    sendResponse(response);
}

void ClientHandler::handleDeleteUser(const QJsonObject &data)
{
    if (!m_currentUser || m_currentUser->getRole() != "admin") {
        QJsonObject response;
        response["type"] = "ERROR";
        response["message"] = "Unauthorized";
        sendResponse(response);
        return;
    }

    int userId = data["user_id"].toInt();
    bool success = DatabaseManager::instance().deleteUser(userId);

    QJsonObject response;
    if (success) {
        response["type"] = "OK";
        response["message"] = "User deleted successfully";
    } else {
        response["type"] = "ERROR";
        response["message"] = "Failed to delete user";
    }
    sendResponse(response);
}

void ClientHandler::handleGetAllUsers()
{
    if (!m_currentUser || m_currentUser->getRole() != "admin") {
        QJsonObject response;
        response["type"] = "ERROR";
        response["message"] = "Unauthorized";
        sendResponse(response);
        return;
    }

    auto users = DatabaseManager::instance().getAllUsers();
    QJsonArray usersArray;
    for (const auto &user : users) {
        usersArray.append(user->toJson());
    }

    QJsonObject response;
    response["type"] = "DATA_RESPONSE";
    response["data"] = usersArray;
    sendResponse(response);
}

void ClientHandler::handleGetMaterials()
{
    auto materials = DatabaseManager::instance().getAllMaterials();
    QJsonArray materialsArray;
    for (const auto &material : materials) {
        materialsArray.append(material->toJson());
    }

    QJsonObject response;
    response["type"] = "DATA_RESPONSE";
    response["data"] = materialsArray;
    sendResponse(response);
}

void ClientHandler::handleCreateQuiz(const QJsonObject &data)
{
    if (!m_currentUser || m_currentUser->getRole() != "instructor") {
        QJsonObject response;
        response["type"] = "ERROR";
        response["message"] = "Unauthorized";
        sendResponse(response);
        return;
    }

    QString title = data["title"].toString();
    int maxAttempts = data["max_attempts"].toInt(1);
    QString feedbackType = data["feedback_type"].toString("detailed_with_answers");

    int quizId = DatabaseManager::instance().createQuiz(title, maxAttempts, feedbackType);

    QJsonObject response;
    if (quizId > 0) {
        response["type"] = "OK";
        response["quiz_id"] = quizId;
        response["message"] = "Quiz created successfully";
    } else {
        response["type"] = "ERROR";
        response["message"] = "Failed to create quiz";
    }
    sendResponse(response);
}

void ClientHandler::handleAddQuestion(const QJsonObject &data)
{
    if (!m_currentUser || m_currentUser->getRole() != "instructor") {
        QJsonObject response;
        response["type"] = "ERROR";
        response["message"] = "Unauthorized";
        sendResponse(response);
        return;
    }

    int quizId = data["quiz_id"].toInt();
    QString prompt = data["prompt"].toString();
    QString questionType = data["question_type"].toString();

    int questionId = DatabaseManager::instance().addQuestion(quizId, prompt, questionType);

    if (questionId > 0 && data.contains("options")) {
        QJsonArray options = data["options"].toArray();
        for (const QJsonValue &val : options) {
            QJsonObject option = val.toObject();
            DatabaseManager::instance().addQuestionOption(questionId,
                                                          option["text"].toString(),
                                                          option["is_correct"].toBool());
        }
    }

    QJsonObject response;
    if (questionId > 0) {
        response["type"] = "OK";
        response["question_id"] = questionId;
        response["message"] = "Question added successfully";
    } else {
        response["type"] = "ERROR";
        response["message"] = "Failed to add question";
    }
    sendResponse(response);
}

void ClientHandler::handleStartQuiz(const QJsonObject &data)
{
    if (!m_currentUser || m_currentUser->getRole() != "student") {
        QJsonObject response;
        response["type"] = "ERROR";
        response["message"] = "Unauthorized";
        sendResponse(response);
        return;
    }

    int quizId = data["quiz_id"].toInt();

    // Check attempt count
    int attemptCount = DatabaseManager::instance().getAttemptCount(quizId, m_currentUser->getId());

    // Get quiz details
    auto quiz = DatabaseManager::instance().getMaterialById(quizId);
    if (!quiz || quiz->getType() != "quiz") {
        QJsonObject response;
        response["type"] = "ERROR";
        response["message"] = "Quiz not found";
        sendResponse(response);
        return;
    }

    QJsonObject response;
    response["type"] = "DATA_RESPONSE";
    response["data"] = quiz->toJson();
    sendResponse(response);
}

void ClientHandler::handleFinishAttempt(const QJsonObject &data)
{
    if (!m_currentUser || m_currentUser->getRole() != "student") {
        QJsonObject response;
        response["type"] = "ERROR";
        response["message"] = "Unauthorized";
        sendResponse(response);
        return;
    }

    int quizId = data["quiz_id"].toInt();
    QJsonArray answers = data["answers"].toArray();

    int attemptNumber = DatabaseManager::instance().getAttemptCount(quizId, m_currentUser->getId())
                        + 1;
    int attemptId = DatabaseManager::instance().createQuizAttempt(quizId,
                                                                  m_currentUser->getId(),
                                                                  attemptNumber);

    if (attemptId < 0) {
        QJsonObject response;
        response["type"] = "ERROR";
        response["message"] = "Failed to create quiz attempt";
        sendResponse(response);
        return;
    }

    // Save answers
    for (const QJsonValue &val : answers) {
        QJsonObject answer = val.toObject();
        DatabaseManager::instance().saveAnswer(attemptId,
                                               answer["question_id"].toInt(),
                                               answer["response"].toString());
    }

    // Auto-grade the attempt
    QJsonObject gradeResult = DatabaseManager::instance().autoGradeQuizAttempt(attemptId);

    if (!gradeResult["success"].toBool()) {
        QJsonObject response;
        response["type"] = "ERROR";
        response["message"] = "Failed to grade quiz attempt";
        sendResponse(response);
        return;
    }

    QJsonObject response;
    response["type"] = "OK";
    response["message"] = "Quiz submitted successfully";
    response["status"] = gradeResult["status"].toString();
    response["auto_score"] = gradeResult["auto_score"].toDouble();
    response["has_open_answers"] = gradeResult["has_open_answers"].toBool();
    response["feedback_type"] = gradeResult["feedback_type"].toString();
    sendResponse(response);
}

void ClientHandler::handleGetMyAttempts()
{
    if (!m_currentUser || m_currentUser->getRole() != "student") {
        QJsonObject response;
        response["type"] = "ERROR";
        response["message"] = "Unauthorized";
        sendResponse(response);
        return;
    }

    QJsonArray attempts = DatabaseManager::instance().getStudentQuizAttempts(m_currentUser->getId());

    QJsonObject response;
    response["type"] = "DATA_RESPONSE";
    response["data"] = attempts;
    sendResponse(response);
}

void ClientHandler::handleGetAttemptDetails(const QJsonObject &data)
{
    if (!m_currentUser) {
        QJsonObject response;
        response["type"] = "ERROR";
        response["message"] = "Unauthorized";
        sendResponse(response);
        return;
    }

    int attemptId = data["attempt_id"].toInt();

    // Students can only view their own attempts
    int studentId = m_currentUser->getRole() == "student" ? m_currentUser->getId() : -1;

    QJsonObject attemptDetails = DatabaseManager::instance().getQuizAttemptDetails(attemptId,
                                                                                   studentId);

    if (attemptDetails.isEmpty()) {
        QJsonObject response;
        response["type"] = "ERROR";
        response["message"] = "Attempt not found or access denied";
        sendResponse(response);
        return;
    }

    QJsonObject response;
    response["type"] = "DATA_RESPONSE";
    response["data"] = attemptDetails;
    sendResponse(response);
}

void ClientHandler::handleGetPendingAttempts()
{
    if (!m_currentUser || m_currentUser->getRole() != "instructor") {
        QJsonObject response;
        response["type"] = "ERROR";
        response["message"] = "Unauthorized";
        sendResponse(response);
        return;
    }

    QJsonArray attempts = DatabaseManager::instance().getPendingAttempts();

    QJsonObject response;
    response["type"] = "DATA_RESPONSE";
    response["data"] = attempts;
    sendResponse(response);
}

void ClientHandler::handleSubmitGrade(const QJsonObject &data)
{
    if (!m_currentUser || m_currentUser->getRole() != "instructor") {
        QJsonObject response;
        response["type"] = "ERROR";
        response["message"] = "Unauthorized";
        sendResponse(response);
        return;
    }

    int attemptId = data["attempt_id"].toInt();
    float score = data["score"].toDouble();

    bool success = DatabaseManager::instance().submitGrade(attemptId, score);

    QJsonObject response;
    if (success) {
        response["type"] = "OK";
        response["message"] = "Grade submitted successfully";
    } else {
        response["type"] = "ERROR";
        response["message"] = "Failed to submit grade";
    }
    sendResponse(response);
}
