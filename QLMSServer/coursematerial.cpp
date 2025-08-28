#include "coursematerial.h"
#include "question.h"
#include <QJsonArray>

CourseMaterial::CourseMaterial(int id, const QString &title)
    : m_id(id)
    , m_title(title)
{}

QJsonObject CourseMaterial::toJson() const
{
    QJsonObject obj;
    obj["material_id"] = m_id;
    obj["title"] = m_title;
    obj["type"] = getType();
    return obj;
}

TextLesson::TextLesson(int id, const QString &title)
    : CourseMaterial(id, title)
{}

QJsonObject TextLesson::toJson() const
{
    QJsonObject obj = CourseMaterial::toJson();
    obj["content"] = m_content;
    return obj;
}

Quiz::Quiz(int id, const QString &title)
    : CourseMaterial(id, title)
{}

void Quiz::addQuestion(std::shared_ptr<Question> question)
{
    m_questions.append(question);
}

QJsonObject Quiz::toJson() const
{
    QJsonObject obj = CourseMaterial::toJson();
    obj["max_attempts"] = m_maxAttempts;

    QJsonArray questionsArray;
    for (const auto &question : m_questions) {
        questionsArray.append(question->toJson());
    }
    obj["questions"] = questionsArray;

    return obj;
}
