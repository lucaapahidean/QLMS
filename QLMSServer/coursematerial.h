#ifndef COURSEMATERIAL_H
#define COURSEMATERIAL_H

#include <memory>
#include <QJsonObject>
#include <QList>
#include <QString>

class Question;

class CourseMaterial
{
public:
    CourseMaterial(int id = 0, const QString &title = QString());
    virtual ~CourseMaterial() = default;

    int getId() const { return m_id; }
    void setId(int id) { m_id = id; }

    QString getTitle() const { return m_title; }
    void setTitle(const QString &title) { m_title = title; }

    virtual QString getType() const = 0;
    virtual QJsonObject toJson() const;

protected:
    int m_id;
    QString m_title;
};

class TextLesson : public CourseMaterial
{
public:
    TextLesson(int id = 0, const QString &title = QString());

    QString getType() const override { return "lesson"; }

    QString getContent() const { return m_content; }
    void setContent(const QString &content) { m_content = content; }

    QJsonObject toJson() const override;

private:
    QString m_content;
};

class Quiz : public CourseMaterial
{
public:
    Quiz(int id = 0, const QString &title = QString());

    QString getType() const override { return "quiz"; }

    int getMaxAttempts() const { return m_maxAttempts; }
    void setMaxAttempts(int attempts) { m_maxAttempts = attempts; }

    void addQuestion(std::shared_ptr<Question> question);
    const QList<std::shared_ptr<Question>> &getQuestions() const { return m_questions; }

    QJsonObject toJson() const override;

private:
    int m_maxAttempts = 1;
    QList<std::shared_ptr<Question>> m_questions;
};

#endif // COURSEMATERIAL_H
