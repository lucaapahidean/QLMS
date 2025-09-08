#include "coursematerial.h"
#include "question.h"
#include <QJsonArray>

CourseMaterial::CourseMaterial(int id, const QString &title, int courseId, int creatorId)
    : m_id(id)
    , m_title(title)
    , m_courseId(courseId)
    , m_creatorId(creatorId)
{}

QJsonObject CourseMaterial::toJson(bool includeAnswers) const
{
    Q_UNUSED(includeAnswers)
    QJsonObject obj;
    obj["material_id"] = m_id;
    obj["title"] = m_title;
    obj["type"] = getType();
    obj["course_id"] = m_courseId;
    obj["creator_id"] = m_creatorId;
    return obj;
}

TextLesson::TextLesson(int id, const QString &title, int courseId, int creatorId)
    : CourseMaterial(id, title, courseId, creatorId)
{}

QJsonObject TextLesson::toJson(bool includeAnswers) const
{
    QJsonObject obj = CourseMaterial::toJson(includeAnswers);
    obj["content"] = m_content;
    return obj;
}

Quiz::Quiz(int id, const QString &title, int courseId, int creatorId)
    : CourseMaterial(id, title, courseId, creatorId)
    , m_maxAttempts(1)
    , m_feedbackType("detailed_with_answers")
{}

void Quiz::addQuestion(std::shared_ptr<Question> question)
{
    m_questions.append(question);
}

QJsonObject Quiz::toJson(bool includeAnswers) const
{
    QJsonObject obj = CourseMaterial::toJson(includeAnswers);
    obj["max_attempts"] = m_maxAttempts;
    obj["feedback_type"] = m_feedbackType;

    QJsonArray questionsArray;
    for (const auto &question : m_questions) {
        questionsArray.append(question->toJson(includeAnswers));
    }
    obj["questions"] = questionsArray;

    return obj;
}
