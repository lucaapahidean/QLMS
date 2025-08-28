#ifndef COURSEEDITORWIDGET_H
#define COURSEEDITORWIDGET_H

#include <QJsonArray>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QListWidget;
class QPushButton;
class QTextEdit;
class QSpinBox;
class QComboBox;
class QLineEdit;
class QTableWidget;
QT_END_NAMESPACE

class CourseEditorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CourseEditorWidget(QWidget *parent = nullptr);

private slots:
    void onRefreshMaterials();
    void onCreateQuiz();
    void onAddQuestion();
    void onQuestionTypeChanged(const QString &type);
    void onAddOption();
    void onRemoveOption();
    void handleMaterialsResponse(const QJsonObject &response);

private:
    void setupUi();
    void populateMaterialsList(const QJsonArray &materials);
    void clearQuestionForm();

    QListWidget *m_materialsListWidget;
    QPushButton *m_refreshButton;
    QPushButton *m_createQuizButton;

    // Quiz creation form
    QLineEdit *m_quizTitleEdit;
    QSpinBox *m_maxAttemptsSpinBox;

    // Question creation form
    QLineEdit *m_currentQuizIdEdit;
    QTextEdit *m_questionPromptEdit;
    QComboBox *m_questionTypeCombo;
    QPushButton *m_addQuestionButton;

    // Options for multiple choice questions
    QTableWidget *m_optionsTable;
    QPushButton *m_addOptionButton;
    QPushButton *m_removeOptionButton;
};

#endif // COURSEEDITORWIDGET_H
