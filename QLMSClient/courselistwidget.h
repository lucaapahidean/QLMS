#ifndef COURSELISTWIDGET_H
#define COURSELISTWIDGET_H

#include <QJsonArray>
#include <QJsonObject>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QListWidget;
class QTextEdit;
class QPushButton;
class QGroupBox;
class QVBoxLayout;
class QTabWidget;
QT_END_NAMESPACE

class CourseListWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CourseListWidget(int studentId, QWidget *parent = nullptr);

private slots:
    void onRefreshMaterials();
    void onRefreshAttempts();
    void onMaterialSelected();
    void onAttemptSelected();
    void onStartQuiz();
    void onSubmitQuiz();
    void handleMaterialsResponse(const QJsonObject &response);
    void handleAttemptsResponse(const QJsonObject &response);
    void handleQuizDataResponse(const QJsonObject &response);
    void handleQuizSubmissionResponse(const QJsonObject &response);
    void handleAttemptDetailsResponse(const QJsonObject &response);

private:
    void setupUi();
    void populateMaterialsList(const QJsonArray &materials);
    void populateAttemptsList(const QJsonArray &attempts);
    void displayTextLesson(const QJsonObject &lesson);
    void displayQuiz(const QJsonObject &quiz);
    void displayAttemptDetails(const QJsonObject &attemptData);
    void clearContentArea();

    int m_studentId;
    QJsonObject m_currentQuiz;
    QJsonArray m_materials;
    QJsonArray m_attempts;

    // Main UI
    QTabWidget *m_tabWidget;

    // Materials tab widgets
    QListWidget *m_materialsListWidget;
    QPushButton *m_refreshButton;
    QGroupBox *m_contentGroup;
    QVBoxLayout *m_contentLayout;
    QTextEdit *m_lessonTextEdit;
    QPushButton *m_startQuizButton;
    QPushButton *m_submitQuizButton;

    // Quiz attempts tab widgets
    QListWidget *m_attemptsListWidget;
    QPushButton *m_refreshAttemptsButton;
    QGroupBox *m_attemptDetailsGroup;
    QVBoxLayout *m_attemptDetailsLayout;
    QTextEdit *m_attemptDetailsText;

    // Quiz-related widgets will be created dynamically
    QList<QObject *> m_quizWidgets;
};

#endif // COURSELISTWIDGET_H
