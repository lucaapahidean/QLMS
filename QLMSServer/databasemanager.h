#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <memory>
#include <QMutex>
#include <QObject>
#include <QSqlDatabase>

class User;
class CourseMaterial;
class Quiz;
class Question;

class DatabaseManager : public QObject
{
    Q_OBJECT

public:
    static DatabaseManager &instance();

    bool initialize(const QString &host,
                    int port,
                    const QString &databaseName,
                    const QString &username,
                    const QString &password);

    // User operations
    std::shared_ptr<User> authenticateUser(const QString &username, const QString &passwordHash);
    std::shared_ptr<User> getUserById(int userId);
    bool createUser(const QString &username, const QString &passwordHash, const QString &role);
    bool deleteUser(int userId);
    QList<std::shared_ptr<User>> getAllUsers();

    // Course material operations
    QList<std::shared_ptr<CourseMaterial>> getAllMaterials();
    std::shared_ptr<CourseMaterial> getMaterialById(int materialId);
    int createQuiz(const QString &title,
                   int maxAttempts,
                   const QString &feedbackType = "detailed_with_answers");
    int addQuestion(int quizId, const QString &prompt, const QString &questionType);
    bool addQuestionOption(int questionId, const QString &text, bool isCorrect);

    // Quiz attempt operations
    int createQuizAttempt(int quizId, int studentId, int attemptNumber);
    bool saveAnswer(int attemptId, int questionId, const QString &response);
    QJsonObject autoGradeQuizAttempt(int attemptId);
    bool finalizeAttempt(int attemptId, const QString &status, float score = -1);
    QJsonArray getPendingAttempts();
    bool submitGrade(int attemptId, float manualScore);
    int getAttemptCount(int quizId, int studentId);

    // New methods for quiz history and attempt details
    QJsonArray getStudentQuizAttempts(int studentId);
    QJsonObject getQuizAttemptDetails(int attemptId, int studentId = -1);

private:
    DatabaseManager();
    ~DatabaseManager();
    DatabaseManager(const DatabaseManager &) = delete;
    DatabaseManager &operator=(const DatabaseManager &) = delete;

    QSqlDatabase getDatabase();
    std::shared_ptr<User> createUserFromQuery(const QSqlQuery &query);
    std::shared_ptr<CourseMaterial> createMaterialFromQuery(const QSqlQuery &query,
                                                            QSqlDatabase &db);
    std::shared_ptr<Quiz> loadQuizDetails(int quizId);
    std::shared_ptr<Question> createQuestionFromQuery(const QSqlQuery &query, QSqlDatabase &db);

    QString m_connectionPrefix;
    QString m_host;
    int m_port;
    QString m_databaseName;
    QString m_username;
    QString m_password;
    QMutex m_mutex;
    int m_connectionCounter;
};

#endif // DATABASEMANAGER_H
