#include "question.h"
#include <QJsonArray>
#include <QStringList>

Question::Question(int id, int quizId, const QString &prompt)
    : m_id(id)
    , m_quizId(quizId)
    , m_prompt(prompt)
{}

QJsonObject Question::toJson() const
{
    QJsonObject obj;
    obj["question_id"] = m_id;
    obj["quiz_id"] = m_quizId;
    obj["prompt"] = m_prompt;
    obj["question_type"] = getType();
    return obj;
}

CheckboxQuestion::CheckboxQuestion(int id, int quizId, const QString &prompt)
    : Question(id, quizId, prompt)
{}

void CheckboxQuestion::addOption(const QString &text, bool isCorrect)
{
    m_options.append(qMakePair(text, isCorrect));
}

QJsonObject CheckboxQuestion::toJson() const
{
    QJsonObject obj = Question::toJson();
    QJsonArray optionsArray;
    for (const auto &option : m_options) {
        QJsonObject optionObj;
        optionObj["text"] = option.first;
        optionObj["is_correct"] = option.second;
        optionsArray.append(optionObj);
    }
    obj["options"] = optionsArray;
    return obj;
}

bool CheckboxQuestion::validateAnswer(const QString &answer) const
{
    QStringList selectedOptions = answer.split(",");
    int correctCount = 0;
    int correctTotal = 0;

    for (int i = 0; i < m_options.size(); ++i) {
        if (m_options[i].second) {
            correctTotal++;
            if (selectedOptions.contains(QString::number(i))) {
                correctCount++;
            }
        } else if (selectedOptions.contains(QString::number(i))) {
            return false; // Selected a wrong option
        }
    }

    return correctCount == correctTotal;
}

RadioButtonQuestion::RadioButtonQuestion(int id, int quizId, const QString &prompt)
    : Question(id, quizId, prompt)
{}

void RadioButtonQuestion::addOption(const QString &text, bool isCorrect)
{
    m_options.append(qMakePair(text, isCorrect));
}

QJsonObject RadioButtonQuestion::toJson() const
{
    QJsonObject obj = Question::toJson();
    QJsonArray optionsArray;
    for (const auto &option : m_options) {
        QJsonObject optionObj;
        optionObj["text"] = option.first;
        optionObj["is_correct"] = option.second;
        optionsArray.append(optionObj);
    }
    obj["options"] = optionsArray;
    return obj;
}

bool RadioButtonQuestion::validateAnswer(const QString &answer) const
{
    bool ok;
    int selectedIndex = answer.toInt(&ok);
    if (!ok || selectedIndex < 0 || selectedIndex >= m_options.size()) {
        return false;
    }
    return m_options[selectedIndex].second;
}

OpenAnswerQuestion::OpenAnswerQuestion(int id, int quizId, const QString &prompt)
    : Question(id, quizId, prompt)
{}

QJsonObject OpenAnswerQuestion::toJson() const
{
    return Question::toJson();
}

bool OpenAnswerQuestion::validateAnswer(const QString &answer) const
{
    Q_UNUSED(answer)
    // Open answer questions require manual grading
    return false;
}
