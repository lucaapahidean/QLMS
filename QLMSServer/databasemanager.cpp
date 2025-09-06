#include "databasemanager.h"
#include "coursematerial.h"
#include "question.h"
#include "user.h"
#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>
#include <QSqlError>
#include <QSqlQuery>
#include <QThread>

DatabaseManager &DatabaseManager::instance()
{
    static DatabaseManager instance;
    return instance;
}

DatabaseManager::DatabaseManager()
    : m_connectionPrefix("QLMSConnection")
    , m_connectionCounter(0)
{}

DatabaseManager::~DatabaseManager()
{
    QStringList connections = QSqlDatabase::connectionNames();
    for (const QString &conn : connections) {
        if (conn.startsWith(m_connectionPrefix)) {
            QSqlDatabase::removeDatabase(conn);
        }
    }
}

bool DatabaseManager::initialize(const QString &host,
                                 int port,
                                 const QString &databaseName,
                                 const QString &username,
                                 const QString &password)
{
    m_host = host;
    m_port = port;
    m_databaseName = databaseName;
    m_username = username;
    m_password = password;

    QSqlDatabase db = getDatabase();
    if (!db.open()) {
        qCritical() << "Failed to open database:" << db.lastError().text();
        return false;
    }

    qInfo() << "Database initialized successfully";
    return true;
}

QSqlDatabase DatabaseManager::getDatabase()
{
    QString connectionName = QString("%1_%2_%3")
                                 .arg(m_connectionPrefix)
                                 .arg(reinterpret_cast<quintptr>(QThread::currentThread()))
                                 .arg(m_connectionCounter++);

    if (!QSqlDatabase::contains(connectionName)) {
        QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL", connectionName);
        db.setHostName(m_host);
        db.setPort(m_port);
        db.setDatabaseName(m_databaseName);
        db.setUserName(m_username);
        db.setPassword(m_password);
    }

    return QSqlDatabase::database(connectionName);
}

std::shared_ptr<User> DatabaseManager::authenticateUser(const QString &username,
                                                        const QString &passwordHash)
{
    QMutexLocker locker(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (!db.open())
        return nullptr;

    QSqlQuery query(db);
    query.prepare("SELECT user_id, username, password_hash, role FROM users "
                  "WHERE username = :username AND password_hash = :password");
    query.bindValue(":username", username);
    query.bindValue(":password", passwordHash);

    if (!query.exec() || !query.next()) {
        return nullptr;
    }

    return createUserFromQuery(query);
}

std::shared_ptr<User> DatabaseManager::getUserById(int userId)
{
    QMutexLocker locker(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (!db.open())
        return nullptr;

    QSqlQuery query(db);
    query.prepare("SELECT user_id, username, password_hash, role FROM users WHERE user_id = :id");
    query.bindValue(":id", userId);

    if (!query.exec() || !query.next()) {
        return nullptr;
    }

    return createUserFromQuery(query);
}

std::shared_ptr<User> DatabaseManager::createUserFromQuery(const QSqlQuery &query)
{
    int id = query.value("user_id").toInt();
    QString username = query.value("username").toString();
    QString passwordHash = query.value("password_hash").toString();
    QString role = query.value("role").toString();

    std::shared_ptr<User> user;
    if (role == "admin") {
        user = std::make_shared<Admin>(id, username);
    } else if (role == "instructor") {
        user = std::make_shared<Instructor>(id, username);
    } else if (role == "student") {
        user = std::make_shared<Student>(id, username);
    } else {
        return nullptr;
    }

    user->setPasswordHash(passwordHash);
    return user;
}

bool DatabaseManager::createUser(const QString &username,
                                 const QString &passwordHash,
                                 const QString &role)
{
    QMutexLocker locker(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (!db.open())
        return false;

    QSqlQuery query(db);
    query.prepare("INSERT INTO users (username, password_hash, role) "
                  "VALUES (:username, :password, :role)");
    query.bindValue(":username", username);
    query.bindValue(":password", passwordHash);
    query.bindValue(":role", role);

    return query.exec();
}

bool DatabaseManager::deleteUser(int userId)
{
    QMutexLocker locker(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (!db.open())
        return false;

    QSqlQuery query(db);
    query.prepare("DELETE FROM users WHERE user_id = :id");
    query.bindValue(":id", userId);

    return query.exec();
}

QList<std::shared_ptr<User>> DatabaseManager::getAllUsers()
{
    QList<std::shared_ptr<User>> users;
    QMutexLocker locker(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (!db.open())
        return users;

    QSqlQuery query(db);
    query.prepare("SELECT user_id, username, password_hash, role FROM users ORDER BY user_id");

    if (!query.exec()) {
        return users;
    }

    while (query.next()) {
        auto user = createUserFromQuery(query);
        if (user) {
            users.append(user);
        }
    }

    return users;
}

QList<std::shared_ptr<CourseMaterial>> DatabaseManager::getAllMaterials()
{
    QList<std::shared_ptr<CourseMaterial>> materials;
    QMutexLocker locker(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (!db.open())
        return materials;

    QSqlQuery query(db);
    query.prepare("SELECT material_id, title, type FROM course_materials ORDER BY material_id");

    if (!query.exec()) {
        return materials;
    }

    while (query.next()) {
        auto material = createMaterialFromQuery(query, db);
        if (material) {
            materials.append(material);
        }
    }

    return materials;
}

std::shared_ptr<CourseMaterial> DatabaseManager::getMaterialById(int materialId)
{
    QMutexLocker locker(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (!db.open())
        return nullptr;

    QSqlQuery query(db);
    query.prepare("SELECT material_id, title, type FROM course_materials WHERE material_id = :id");
    query.bindValue(":id", materialId);

    if (!query.exec() || !query.next()) {
        return nullptr;
    }

    return createMaterialFromQuery(query, db);
}

std::shared_ptr<CourseMaterial> DatabaseManager::createMaterialFromQuery(const QSqlQuery &query,
                                                                         QSqlDatabase &db)
{
    int id = query.value("material_id").toInt();
    QString title = query.value("title").toString();
    QString type = query.value("type").toString();

    if (type == "lesson") {
        auto lesson = std::make_shared<TextLesson>(id, title);

        // Load lesson content
        QSqlQuery contentQuery(db);
        contentQuery.prepare("SELECT content FROM text_lessons WHERE lesson_id = :id");
        contentQuery.bindValue(":id", id);
        if (contentQuery.exec() && contentQuery.next()) {
            lesson->setContent(contentQuery.value("content").toString());
        }

        return lesson;
    } else if (type == "quiz") {
        return loadQuizDetails(id);
    }

    return nullptr;
}

std::shared_ptr<Quiz> DatabaseManager::loadQuizDetails(int quizId)
{
    QSqlDatabase db = getDatabase();

    // Get quiz basic info
    QSqlQuery quizQuery(db);
    quizQuery.prepare("SELECT cm.title, q.max_attempts "
                      "FROM course_materials cm "
                      "JOIN quizzes q ON cm.material_id = q.quiz_id "
                      "WHERE q.quiz_id = :id");
    quizQuery.bindValue(":id", quizId);

    if (!quizQuery.exec() || !quizQuery.next()) {
        return nullptr;
    }

    auto quiz = std::make_shared<Quiz>(quizId, quizQuery.value("title").toString());
    quiz->setMaxAttempts(quizQuery.value("max_attempts").toInt());

    // Load questions
    QSqlQuery questionsQuery(db);
    questionsQuery.prepare("SELECT question_id, quiz_id, prompt, question_type "
                           "FROM questions WHERE quiz_id = :id ORDER BY question_id");
    questionsQuery.bindValue(":id", quizId);

    if (questionsQuery.exec()) {
        while (questionsQuery.next()) {
            auto question = createQuestionFromQuery(questionsQuery, db);
            if (question) {
                quiz->addQuestion(question);
            }
        }
    }

    return quiz;
}

std::shared_ptr<Question> DatabaseManager::createQuestionFromQuery(const QSqlQuery &query,
                                                                   QSqlDatabase &db)
{
    int id = query.value("question_id").toInt();
    int quizId = query.value("quiz_id").toInt();
    QString prompt = query.value("prompt").toString();
    QString type = query.value("question_type").toString();

    std::shared_ptr<Question> question;

    if (type == "checkbox") {
        question = std::make_shared<CheckboxQuestion>(id, quizId, prompt);
    } else if (type == "radio") {
        question = std::make_shared<RadioButtonQuestion>(id, quizId, prompt);
    } else if (type == "open_answer") {
        question = std::make_shared<OpenAnswerQuestion>(id, quizId, prompt);
    } else {
        return nullptr;
    }

    // Load options for checkbox and radio questions
    if (type == "checkbox" || type == "radio") {
        QSqlQuery optionsQuery(db);
        optionsQuery.prepare("SELECT option_text, is_correct FROM question_options "
                             "WHERE question_id = :id ORDER BY option_id");
        optionsQuery.bindValue(":id", id);

        if (optionsQuery.exec()) {
            while (optionsQuery.next()) {
                QString optionText = optionsQuery.value("option_text").toString();
                bool isCorrect = optionsQuery.value("is_correct").toBool();

                if (auto cbQuestion = std::dynamic_pointer_cast<CheckboxQuestion>(question)) {
                    cbQuestion->addOption(optionText, isCorrect);
                } else if (auto rbQuestion = std::dynamic_pointer_cast<RadioButtonQuestion>(
                               question)) {
                    rbQuestion->addOption(optionText, isCorrect);
                }
            }
        }
    }

    return question;
}

int DatabaseManager::createQuiz(const QString &title, int maxAttempts)
{
    QMutexLocker locker(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (!db.open())
        return -1;

    db.transaction();

    QSqlQuery query(db);
    query.prepare(
        "INSERT INTO course_materials (title, type) VALUES (:title, 'quiz') RETURNING material_id");
    query.bindValue(":title", title);

    if (!query.exec()) {
        qCritical() << "Failed to insert course material:" << query.lastError().text();
        db.rollback();
        return -1;
    }

    if (!query.next()) {
        qCritical() << "Failed to get material_id from INSERT";
        db.rollback();
        return -1;
    }

    int materialId = query.value(0).toInt();

    query.prepare("INSERT INTO quizzes (quiz_id, max_attempts) VALUES (:id, :attempts)");
    query.bindValue(":id", materialId);
    query.bindValue(":attempts", maxAttempts);

    if (!query.exec()) {
        db.rollback();
        return -1;
    }

    db.commit();
    return materialId;
}

int DatabaseManager::addQuestion(int quizId, const QString &prompt, const QString &questionType)
{
    QMutexLocker locker(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (!db.open())
        return -1;

    QSqlQuery query(db);
    query.prepare("INSERT INTO questions (quiz_id, prompt, question_type) "
                  "VALUES (:quiz_id, :prompt, :type) RETURNING question_id");
    query.bindValue(":quiz_id", quizId);
    query.bindValue(":prompt", prompt);
    query.bindValue(":type", questionType);

    if (!query.exec() || !query.next()) {
        return -1;
    }

    return query.value(0).toInt();
}

bool DatabaseManager::addQuestionOption(int questionId, const QString &text, bool isCorrect)
{
    QMutexLocker locker(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (!db.open())
        return false;

    QSqlQuery query(db);
    query.prepare("INSERT INTO question_options (question_id, option_text, is_correct) "
                  "VALUES (:question_id, :text, :correct)");
    query.bindValue(":question_id", questionId);
    query.bindValue(":text", text);
    query.bindValue(":correct", isCorrect);

    return query.exec();
}

int DatabaseManager::createQuizAttempt(int quizId, int studentId, int attemptNumber)
{
    QMutexLocker locker(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (!db.open())
        return -1;

    QSqlQuery query(db);
    query.prepare("INSERT INTO quiz_attempts (quiz_id, student_id, attempt_number, status) "
                  "VALUES (:quiz_id, :student_id, :attempt, 'pending_manual_grading') "
                  "RETURNING attempt_id");
    query.bindValue(":quiz_id", quizId);
    query.bindValue(":student_id", studentId);
    query.bindValue(":attempt", attemptNumber);

    if (!query.exec() || !query.next()) {
        return -1;
    }

    return query.value(0).toInt();
}

bool DatabaseManager::saveAnswer(int attemptId, int questionId, const QString &response)
{
    QMutexLocker locker(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (!db.open())
        return false;

    QSqlQuery query(db);
    query.prepare("INSERT INTO answers (attempt_id, question_id, student_response) "
                  "VALUES (:attempt_id, :question_id, :response)");
    query.bindValue(":attempt_id", attemptId);
    query.bindValue(":question_id", questionId);
    query.bindValue(":response", response);

    return query.exec();
}

bool DatabaseManager::finalizeAttempt(int attemptId, const QString &status, float score)
{
    QMutexLocker locker(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (!db.open())
        return false;

    QSqlQuery query(db);
    if (score >= 0) {
        query.prepare("UPDATE quiz_attempts SET status = :status, final_score = :score "
                      "WHERE attempt_id = :id");
        query.bindValue(":score", score);
    } else {
        query.prepare("UPDATE quiz_attempts SET status = :status WHERE attempt_id = :id");
    }
    query.bindValue(":status", status);
    query.bindValue(":id", attemptId);

    return query.exec();
}

QJsonArray DatabaseManager::getPendingAttempts()
{
    QJsonArray attempts;
    QMutexLocker locker(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (!db.open())
        return attempts;

    QSqlQuery query(db);
    query.prepare("SELECT qa.attempt_id, qa.quiz_id, qa.student_id, qa.attempt_number, "
                  "u.username, cm.title "
                  "FROM quiz_attempts qa "
                  "JOIN users u ON qa.student_id = u.user_id "
                  "JOIN course_materials cm ON qa.quiz_id = cm.material_id "
                  "WHERE qa.status = 'pending_manual_grading' "
                  "ORDER BY qa.attempt_id");

    if (!query.exec()) {
        return attempts;
    }

    while (query.next()) {
        QJsonObject obj;
        obj["attempt_id"] = query.value("attempt_id").toInt();
        obj["quiz_id"] = query.value("quiz_id").toInt();
        obj["student_id"] = query.value("student_id").toInt();
        obj["attempt_number"] = query.value("attempt_number").toInt();
        obj["student_name"] = query.value("username").toString();
        obj["quiz_title"] = query.value("title").toString();
        attempts.append(obj);
    }

    return attempts;
}

bool DatabaseManager::submitGrade(int attemptId, float score)
{
    return finalizeAttempt(attemptId, "completed", score);
}

int DatabaseManager::getAttemptCount(int quizId, int studentId)
{
    QMutexLocker locker(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (!db.open())
        return 0;

    QSqlQuery query(db);
    query.prepare("SELECT COUNT(*) FROM quiz_attempts "
                  "WHERE quiz_id = :quiz_id AND student_id = :student_id");
    query.bindValue(":quiz_id", quizId);
    query.bindValue(":student_id", studentId);

    if (!query.exec() || !query.next()) {
        return 0;
    }

    return query.value(0).toInt();
}
