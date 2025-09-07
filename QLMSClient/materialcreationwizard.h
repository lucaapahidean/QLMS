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
class QListWidgetItem;
class QPushButton;
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
    QString content() const;

private:
    QTextEdit *m_contentEdit;
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
    void onAddOrUpdateQuestion();
    void onNewQuestion();
    void onRemoveQuestion();
    void onQuestionSelected(QListWidgetItem *item);
    void onQuestionTypeChanged(const QString &type);
    void onAddOption();
    void onRemoveOption();

private:
    void loadQuestionForEditing(int index);
    void clearEditor();
    QJsonObject createQuestionFromEditor();

    QListWidget *m_questionsList;
    QPushButton *m_newQuestionButton;
    QPushButton *m_addOrUpdateQuestionButton;
    QPushButton *m_removeQuestionButton;

    // Question details
    QTextEdit *m_questionPromptEdit;
    QComboBox *m_questionTypeCombo;
    QTableWidget *m_optionsTable;
    QPushButton *m_addOptionButton;
    QPushButton *m_removeOptionButton;

    QJsonArray m_questions;
    int m_currentQuestionIndex = -1; // -1 means new question, otherwise it's the index
};

class ConclusionPage : public QWizardPage
{
    Q_OBJECT

public:
    ConclusionPage(QWidget *parent = nullptr);
    void initializePage() override;
};

#endif // MATERIALCREATIONWIZARD_H
