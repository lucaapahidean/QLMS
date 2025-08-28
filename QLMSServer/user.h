#ifndef USER_H
#define USER_H

#include <QJsonObject>
#include <QString>

class User
{
public:
    User(int id = 0, const QString &username = QString());
    virtual ~User() = default;

    int getId() const { return m_id; }
    void setId(int id) { m_id = id; }

    QString getUsername() const { return m_username; }
    void setUsername(const QString &username) { m_username = username; }

    QString getPasswordHash() const { return m_passwordHash; }
    void setPasswordHash(const QString &hash) { m_passwordHash = hash; }

    virtual QString getRole() const = 0;
    virtual QJsonObject toJson() const;

protected:
    int m_id;
    QString m_username;
    QString m_passwordHash;
};

class Admin : public User
{
public:
    Admin(int id = 0, const QString &username = QString());
    QString getRole() const override { return "admin"; }
};

class Instructor : public User
{
public:
    Instructor(int id = 0, const QString &username = QString());
    QString getRole() const override { return "instructor"; }
};

class Student : public User
{
public:
    Student(int id = 0, const QString &username = QString());
    QString getRole() const override { return "student"; }
};

#endif // USER_H
