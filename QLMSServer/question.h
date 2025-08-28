#ifndef QUESTION_H
#define QUESTION_H

#include <QJsonObject>
#include <QList>
#include <QPair>
#include <QString>

class Question
{
public:
    Question(int id = 0, int quizId = 0, const QString &prompt = QString());
    virtual ~Question() = default;

    int getId() const { return m_id; }
    void setId(int id) { m_id = id; }

    int getQuizId() const { return m_quizId; }
    void setQuizId(int id) { m_quizId = id; }

    QString getPrompt() const { return m_prompt; }
    void setPrompt(const QString &prompt) { m_prompt = prompt; }

    virtual QString getType() const = 0;
    virtual QJsonObject toJson() const;
    virtual bool validateAnswer(const QString &answer) const = 0;

protected:
    int m_id;
    int m_quizId;
    QString m_prompt;
};

class CheckboxQuestion : public Question
{
public:
    CheckboxQuestion(int id = 0, int quizId = 0, const QString &prompt = QString());

    QString getType() const override { return "checkbox"; }

    void addOption(const QString &text, bool isCorrect);
    const QList<QPair<QString, bool>> &getOptions() const { return m_options; }

    QJsonObject toJson() const override;
    bool validateAnswer(const QString &answer) const override;

private:
    QList<QPair<QString, bool>> m_options;
};

class RadioButtonQuestion : public Question
{
public:
    RadioButtonQuestion(int id = 0, int quizId = 0, const QString &prompt = QString());

    QString getType() const override { return "radio"; }

    void addOption(const QString &text, bool isCorrect);
    const QList<QPair<QString, bool>> &getOptions() const { return m_options; }

    QJsonObject toJson() const override;
    bool validateAnswer(const QString &answer) const override;

private:
    QList<QPair<QString, bool>> m_options;
};

class OpenAnswerQuestion : public Question
{
public:
    OpenAnswerQuestion(int id = 0, int quizId = 0, const QString &prompt = QString());

    QString getType() const override { return "open_answer"; }

    QJsonObject toJson() const override;
    bool validateAnswer(const QString &answer) const override;
};

#endif // QUESTION_H
