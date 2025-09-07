#include "gradingwidget.h"
#include "networkmanager.h"
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTextEdit>
#include <QVBoxLayout>

GradingWidget::GradingWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
    onRefreshPendingAttempts();
}

void GradingWidget::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);

    // Header
    auto *headerLayout = new QHBoxLayout();
    auto *titleLabel = new QLabel("Quiz Grading", this);
    QFont font = titleLabel->font();
    font.setPointSize(14);
    font.setBold(true);
    titleLabel->setFont(font);
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();

    m_refreshButton = new QPushButton("Refresh", this);
    headerLayout->addWidget(m_refreshButton);

    mainLayout->addLayout(headerLayout);

    // Pending attempts table
    mainLayout->addWidget(new QLabel("Pending Manual Grading:", this));

    m_attemptsTable = new QTableWidget(this);
    m_attemptsTable->setColumnCount(7);
    m_attemptsTable->setHorizontalHeaderLabels(
        QStringList() << "Attempt ID" << "Student" << "Quiz Title" << "Attempt #"
                      << "Auto Score" << "Open Questions" << "Status");
    m_attemptsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_attemptsTable->setAlternatingRowColors(true);
    m_attemptsTable->horizontalHeader()->setStretchLastSection(true);
    mainLayout->addWidget(m_attemptsTable);

    // Score info group
    auto *scoreInfoGroup = new QGroupBox("Scoring Information", this);
    auto *scoreInfoLayout = new QVBoxLayout(scoreInfoGroup);

    m_scoreInfoLabel = new QLabel(this);
    m_scoreInfoLabel->setWordWrap(true);
    scoreInfoLayout->addWidget(m_scoreInfoLabel);

    m_questionAnswerTextEdit = new QTextEdit(this);
    m_questionAnswerTextEdit->setReadOnly(true);
    scoreInfoLayout->addWidget(m_questionAnswerTextEdit);

    mainLayout->addWidget(scoreInfoGroup);

    // Grading controls
    auto *gradingGroup = new QGroupBox("Submit Manual Grade", this);
    auto *gradingLayout = new QHBoxLayout(gradingGroup);

    gradingLayout->addWidget(new QLabel("Score for Open Answer Questions:", this));

    m_scoreSpinBox = new QDoubleSpinBox(this);
    m_scoreSpinBox->setRange(0.0, 100.0);
    m_scoreSpinBox->setSuffix("%");
    m_scoreSpinBox->setDecimals(2);
    m_scoreSpinBox->setToolTip("Enter the percentage score for the open answer questions only");
    gradingLayout->addWidget(m_scoreSpinBox);

    m_submitGradeButton = new QPushButton("Submit Grade", this);
    m_submitGradeButton->setEnabled(false);
    gradingLayout->addWidget(m_submitGradeButton);

    gradingLayout->addStretch();

    mainLayout->addWidget(gradingGroup);
    mainLayout->addStretch();

    // Connect signals
    connect(m_refreshButton, &QPushButton::clicked, this, &GradingWidget::onRefreshPendingAttempts);
    connect(m_submitGradeButton, &QPushButton::clicked, this, &GradingWidget::onSubmitGrade);
    connect(m_attemptsTable, &QTableWidget::itemSelectionChanged, this, [this]() {
        bool hasSelection = m_attemptsTable->currentRow() >= 0;
        m_submitGradeButton->setEnabled(hasSelection);

        if (hasSelection) {
            updateScoreInfo();
        } else {
            m_scoreInfoLabel->clear();
            m_questionAnswerTextEdit->clear();
        }
    });
}

void GradingWidget::updateScoreInfo()
{
    int row = m_attemptsTable->currentRow();
    if (row < 0)
        return;

    QJsonObject attempt = m_pendingAttempts[row].toObject();
    QString autoScore = m_attemptsTable->item(row, 4)->text();
    QString openQuestions = m_attemptsTable->item(row, 5)->text();
    QString studentName = m_attemptsTable->item(row, 1)->text();

    QString info = QString("Student: %1\n"
                           "Auto-graded score: %2\n"
                           "Number of open answer questions: %3\n\n"
                           "Enter the manual score for the open answer questions. "
                           "The final grade will be calculated as a weighted average.")
                       .arg(studentName)
                       .arg(autoScore)
                       .arg(openQuestions);

    m_scoreInfoLabel->setText(info);

    QJsonArray questions = attempt["questions"].toArray();
    QString qaText;
    for (const QJsonValue &value : questions) {
        QJsonObject question = value.toObject();
        qaText += "Question:\n" + question["prompt"].toString() + "\n\n";
        qaText += "Student's Answer:\n" + question["student_response"].toString() + "\n\n";
    }
    m_questionAnswerTextEdit->setPlainText(qaText);
}

void GradingWidget::onRefreshPendingAttempts()
{
    NetworkManager::instance().sendCommand("GET_PENDING_ATTEMPTS",
                                           QJsonObject(),
                                           [this](const QJsonObject &response) {
                                               handlePendingAttemptsResponse(response);
                                           });
}

void GradingWidget::onSubmitGrade()
{
    int row = m_attemptsTable->currentRow();
    if (row < 0)
        return;

    int attemptId = m_attemptsTable->item(row, 0)->text().toInt();
    QString studentName = m_attemptsTable->item(row, 1)->text();
    QString autoScore = m_attemptsTable->item(row, 4)->text();

    int ret = QMessageBox::question(this,
                                    "Submit Grade",
                                    QString(
                                        "Submit manual grade of %1% for open answer questions?\n\n"
                                        "Student: %2\n"
                                        "Auto-graded score: %3\n"
                                        "The final grade will be calculated automatically.")
                                        .arg(QString::number(m_scoreSpinBox->value(), 'f', 2))
                                        .arg(studentName)
                                        .arg(autoScore),
                                    QMessageBox::Yes | QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        QJsonObject data;
        data["attempt_id"] = attemptId;
        data["score"] = m_scoreSpinBox->value();

        NetworkManager::instance()
            .sendCommand("SUBMIT_GRADE", data, [this](const QJsonObject &response) {
                if (response["type"].toString() == "OK") {
                    QMessageBox::information(this, "Success", "Grade submitted successfully");
                    onRefreshPendingAttempts();
                    m_scoreInfoLabel->clear();
                    m_questionAnswerTextEdit->clear();
                } else {
                    QMessageBox::critical(this, "Error", response["message"].toString());
                }
            });
    }
}

void GradingWidget::handlePendingAttemptsResponse(const QJsonObject &response)
{
    if (response["type"].toString() == "DATA_RESPONSE") {
        m_pendingAttempts = response["data"].toArray();
        populateAttemptsTable(m_pendingAttempts);
    } else {
        QMessageBox::critical(this, "Error", "Failed to fetch pending attempts");
    }
}

void GradingWidget::populateAttemptsTable(const QJsonArray &attempts)
{
    m_attemptsTable->setRowCount(0);

    for (const QJsonValue &value : attempts) {
        QJsonObject attempt = value.toObject();
        int row = m_attemptsTable->rowCount();
        m_attemptsTable->insertRow(row);

        m_attemptsTable
            ->setItem(row, 0, new QTableWidgetItem(QString::number(attempt["attempt_id"].toInt())));
        m_attemptsTable->setItem(row, 1, new QTableWidgetItem(attempt["student_name"].toString()));
        m_attemptsTable->setItem(row, 2, new QTableWidgetItem(attempt["quiz_title"].toString()));
        m_attemptsTable->setItem(row,
                                 3,
                                 new QTableWidgetItem(
                                     QString::number(attempt["attempt_number"].toInt())));

        // Show auto score
        double autoScore = attempt["auto_score"].toDouble();
        m_attemptsTable->setItem(row,
                                 4,
                                 new QTableWidgetItem(QString("%1%").arg(autoScore, 0, 'f', 1)));

        // Show number of open questions
        int manualQuestions = attempt["total_manual_points"].toInt();
        m_attemptsTable->setItem(row, 5, new QTableWidgetItem(QString::number(manualQuestions)));

        m_attemptsTable->setItem(row, 6, new QTableWidgetItem("Pending"));
    }

    m_attemptsTable->resizeColumnsToContents();
}
