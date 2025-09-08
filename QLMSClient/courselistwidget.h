#ifndef COURSELISTWIDGET_H
#define COURSELISTWIDGET_H

#include <QJsonArray>
#include <QJsonObject>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QTreeWidget;
class QTreeWidgetItem;
class QTextEdit;
class QPushButton;
class QGroupBox;
class QVBoxLayout;
class QTabWidget;
class QLineEdit;
QT_END_NAMESPACE

class CourseListWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CourseListWidget(int studentId, QWidget *parent = nullptr);

private slots:
    void onRefresh();
    void onItemSelected();
    void onAttemptSelected();
    void onStartQuiz();
    void onSubmitQuiz();
    void handleClassesResponse(const QJsonObject &response);
    void handleCoursesResponse(const QJsonObject &response, QTreeWidgetItem *classItem);
    void handleMaterialsResponse(const QJsonObject &response, QTreeWidgetItem *courseItem);
    void handleQuizDataResponse(const QJsonObject &response);
    void handleQuizSubmissionResponse(const QJsonObject &response);
    void handleAttemptDetailsResponse(const QJsonObject &response);
    void applyMaterialsFilter(const QString &text);
    void applyAttemptsFilter(const QString &text);

private:
    void setupUi();
    void displayTextLesson(const QJsonObject &lesson);
    void displayQuiz(const QJsonObject &quiz);
    void displayAttemptDetails(const QJsonObject &attemptData);
    void clearContentArea();
    void populateAttemptsList();

    int m_studentId;
    QJsonObject m_currentQuiz;
    QJsonArray m_allAttempts;

    // Main UI
    QTabWidget *m_tabWidget;

    // Materials tab widgets
    QTreeWidget *m_materialsTreeWidget;
    QPushButton *m_refreshButton;
    QLineEdit *m_materialsFilterEdit;
    QGroupBox *m_contentGroup;
    QVBoxLayout *m_contentLayout;
    QTextEdit *m_lessonTextEdit;
    QPushButton *m_startQuizButton;
    QPushButton *m_submitQuizButton;

    // Quiz attempts tab widgets
    QTreeWidget *m_attemptsTreeWidget;
    QPushButton *m_refreshAttemptsButton;
    QLineEdit *m_attemptsFilterEdit;
    QGroupBox *m_attemptDetailsGroup;
    QVBoxLayout *m_attemptDetailsLayout;
    QTextEdit *m_attemptDetailsText;

    // Quiz-related widgets will be created dynamically
    QList<QObject *> m_quizWidgets;
};

#endif // COURSELISTWIDGET_H
