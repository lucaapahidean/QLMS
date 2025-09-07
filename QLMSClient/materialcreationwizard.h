#ifndef MATERIALCREATIONWIZARD_H
#define MATERIALCREATIONWIZARD_H

#include <QJsonArray>
#include <QWizard>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QTextEdit;
class QSpinBox;
class QComboBox;
class QTableWidget;
class QListWidget;
QT_END_NAMESPACE

class MaterialCreationWizard : public QWizard
{
    Q_OBJECT

public:
    explicit MaterialCreationWizard(QWidget *parent = nullptr);

    enum { Page_Intro, Page_Lesson, Page_Quiz, Page_Questions, Page_Conclusion };

    void accept() override;

private:
    void sendDataToServer();
};

class IntroPage : public QWizardPage
{
    Q_OBJECT

public:
    IntroPage(QWidget *parent = nullptr);
    int nextId() const override;
};

class LessonPage : public QWizardPage
{
    Q_OBJECT

public:
    LessonPage(QWidget *parent = nullptr);
    int nextId() const override;
};

class QuizPage : public QWizardPage
{
    Q_OBJECT

public:
    QuizPage(QWidget *parent = nullptr);
    int nextId() const override;
};

class QuestionsPage : public QWizardPage
{
    Q_OBJECT
    Q_PROPERTY(QJsonArray questionsJson READ questionsJson NOTIFY completeChanged)

public:
    QuestionsPage(QWidget *parent = nullptr);
    bool isComplete() const override;
    QJsonArray questionsJson() const { return m_questions; }
    int nextId() const override;

private slots:
    void onAddQuestion();
    void onRemoveQuestion();
    void onQuestionTypeChanged(const QString &type);
    void onAddOption();
    void onRemoveOption();

private:
    QListWidget *m_questionsList;
    QPushButton *m_addQuestionButton;
    QPushButton *m_removeQuestionButton;

    // Question details
    QTextEdit *m_questionPromptEdit;
    QComboBox *m_questionTypeCombo;
    QTableWidget *m_optionsTable;
    QPushButton *m_addOptionButton;
    QPushButton *m_removeOptionButton;

    QJsonArray m_questions;
};

class ConclusionPage : public QWizardPage
{
    Q_OBJECT

public:
    ConclusionPage(QWidget *parent = nullptr);
    void initializePage() override;
};

#endif // MATERIALCREATIONWIZARD_H
