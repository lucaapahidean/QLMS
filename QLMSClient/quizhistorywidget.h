#ifndef QUIZHISTORYWIDGET_H
#define QUIZHISTORYWIDGET_H

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

class QuizHistoryWidget : public QWidget
{
    Q_OBJECT

public:
    explicit QuizHistoryWidget(int studentId, QWidget *parent = nullptr);

private slots:
    void onRefresh();
    void onAttemptSelected();
    void handleAttemptDetailsResponse(const QJsonObject &response);
    void applyFilter();

private:
    void setupUi();
    void displayAttemptDetails(const QJsonObject &attemptData);
    void populateAttemptsList();
    bool applyFilterRecursive(QTreeWidgetItem *item, const QString &text, const QString &filterBy);

    int m_studentId;
    QJsonArray m_allAttempts;

    // UI Elements
    FilterWidget *m_attemptsFilterWidget;
    QTreeWidget *m_attemptsTreeWidget;
    QPushButton *m_refreshButton;
    QGroupBox *m_attemptDetailsGroup;
    QTextEdit *m_attemptDetailsText;
};

#endif // QUIZHISTORYWIDGET_H
