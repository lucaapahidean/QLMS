#include "user.h"

User::User(int id, const QString &username)
    : m_id(id)
    , m_username(username)
{}

QJsonObject User::toJson() const
{
    QJsonObject obj;
    obj["user_id"] = m_id;
    obj["username"] = m_username;
    obj["role"] = getRole();
    return obj;
}

Admin::Admin(int id, const QString &username)
    : User(id, username)
{}

Instructor::Instructor(int id, const QString &username)
    : User(id, username)
{}

Student::Student(int id, const QString &username)
    : User(id, username)
{}
