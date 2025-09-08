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
QT_END_NAMESPACE

class FilterWidget;

class CourseListWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CourseListWidget(int studentId, QWidget *parent = nullptr);

private slots:
    void onRefresh();
    void onItemSelected();
    void onStartQuiz();
    void onSubmitQuiz();
    void handleClassesResponse(const QJsonObject &response);
    void handleCoursesResponse(const QJsonObject &response, QTreeWidgetItem *classItem);
    void handleMaterialsResponse(const QJsonObject &response, QTreeWidgetItem *courseItem);
    void handleQuizDataResponse(const QJsonObject &response);
    void handleQuizSubmissionResponse(const QJsonObject &response);
    void applyFilter();

private:
    void setupUi();
    void displayTextLesson(const QJsonObject &lesson);
    void displayQuiz(const QJsonObject &quiz);
    void clearContentArea();
    bool applyFilterRecursive(QTreeWidgetItem *item, const QString &text, const QString &filterBy);

    int m_studentId;
    QJsonObject m_currentQuiz;

    // UI Elements
    FilterWidget *m_materialsFilterWidget;
    QTreeWidget *m_materialsTreeWidget;
    QPushButton *m_refreshButton;
    QGroupBox *m_contentGroup;
    QVBoxLayout *m_contentLayout;
    QTextEdit *m_lessonTextEdit;
    QPushButton *m_startQuizButton;
    QPushButton *m_submitQuizButton;

    // Quiz-related widgets will be created dynamically
    QList<QObject *> m_quizWidgets;
};

#endif // COURSELISTWIDGET_H
