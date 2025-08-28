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
QT_END_NAMESPACE

class CourseListWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CourseListWidget(int studentId, QWidget *parent = nullptr);

private slots:
    void onRefreshMaterials();
    void onMaterialSelected();
    void onStartQuiz();
    void onSubmitQuiz();
    void handleMaterialsResponse(const QJsonObject &response);
    void handleQuizDataResponse(const QJsonObject &response);

private:
    void setupUi();
    void populateMaterialsList(const QJsonArray &materials);
    void displayTextLesson(const QJsonObject &lesson);
    void displayQuiz(const QJsonObject &quiz);
    void clearContentArea();

    int m_studentId;
    QJsonObject m_currentQuiz;
    QJsonArray m_materials;

    QListWidget *m_materialsListWidget;
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
