#ifndef GRADINGWIDGET_H
#define GRADINGWIDGET_H

#include <QJsonArray>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QTableWidget;
class QPushButton;
class QDoubleSpinBox;
class QLabel;
class QTextEdit;
QT_END_NAMESPACE

class GradingWidget : public QWidget
{
    Q_OBJECT

public:
    explicit GradingWidget(QWidget *parent = nullptr);

private slots:
    void onRefreshPendingAttempts();
    void onSubmitGrade();
    void handlePendingAttemptsResponse(const QJsonObject &response);

private:
    void setupUi();
    void populateAttemptsTable(const QJsonArray &attempts);
    void updateScoreInfo();

    QTableWidget *m_attemptsTable;
    QPushButton *m_refreshButton;
    QDoubleSpinBox *m_scoreSpinBox;
    QPushButton *m_submitGradeButton;
    QLabel *m_scoreInfoLabel;
    QTextEdit *m_questionAnswerTextEdit;
    QJsonArray m_pendingAttempts;
};

#endif // GRADINGWIDGET_H
