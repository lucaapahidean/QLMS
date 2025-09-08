#include "gradingwidget.h"
#include "networkmanager.h"
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>
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

    auto *splitter = new QSplitter(Qt::Vertical, this);

    // Top Panel: Pending attempts table
    auto *topWidget = new QWidget(this);
    auto *topLayout = new QVBoxLayout(topWidget);
    topLayout->addWidget(new QLabel("Pending Manual Grading:", this));
    m_attemptsTable = new QTableWidget(this);
    m_attemptsTable->setColumnCount(10);
    m_attemptsTable->setHorizontalHeaderLabels(
        QStringList() << "Attempt ID" << "Question ID" << "Student" << "Quiz Title"
                      << "Course" << "Class" << "Attempt #"
                      << "Auto Score" << "Question #" << "Status");
    m_attemptsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_attemptsTable->setAlternatingRowColors(true);

    for (int i = 0; i < m_attemptsTable->columnCount(); ++i) {
        if (i == 8) { // "Question #" column
            m_attemptsTable->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Stretch);
        } else {
            m_attemptsTable->horizontalHeader()->setSectionResizeMode(i,
                                                                      QHeaderView::ResizeToContents);
        }
    }

    m_attemptsTable->setColumnHidden(0, true); // Attempt ID
    m_attemptsTable->setColumnHidden(1, true); // Question ID
    m_attemptsTable->setColumnHidden(9, true); // Status
    topLayout->addWidget(m_attemptsTable);
    splitter->addWidget(topWidget);

    // Bottom Panel: Score info and grading controls
    auto *bottomWidget = new QWidget(this);
    auto *bottomLayout = new QVBoxLayout(bottomWidget);

    auto *scoreInfoGroup = new QGroupBox("Scoring Information", this);
    auto *scoreInfoLayout = new QVBoxLayout(scoreInfoGroup);

    m_scoreInfoLabel = new QLabel(this);
    m_scoreInfoLabel->setWordWrap(true);
    scoreInfoLayout->addWidget(m_scoreInfoLabel);

    m_questionAnswerTextEdit = new QTextEdit(this);
    m_questionAnswerTextEdit->setReadOnly(true);
    scoreInfoLayout->addWidget(m_questionAnswerTextEdit);
    bottomLayout->addWidget(scoreInfoGroup);

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
    bottomLayout->addWidget(gradingGroup);
    splitter->addWidget(bottomWidget);

    mainLayout->addWidget(splitter);

    // Connect signals
    connect(m_refreshButton, &QPushButton::clicked, this, &GradingWidget::onRefreshPendingAttempts);
    connect(m_submitGradeButton, &QPushButton::clicked, this, &GradingWidget::onSubmitGrade);
    connect(m_attemptsTable, &QTableWidget::itemSelectionChanged, this, [this]() {
        bool hasSelection = m_attemptsTable->currentRow() >= 0;
        m_submitGradeButton->setEnabled(hasSelection);

        if (hasSelection) {
            updateScoreInfo(m_attemptsTable->currentRow());
        } else {
            m_scoreInfoLabel->clear();
            m_questionAnswerTextEdit->clear();
        }
    });
}

void GradingWidget::updateScoreInfo(int row)
{
    if (row < 0)
        return;

    QJsonObject attempt = m_pendingAttempts[row].toObject();
    QString studentName = attempt["student_name"].toString();
    QString quizTitle = attempt["quiz_title"].toString();
    QString courseName = attempt["course_name"].toString();
    QString className = attempt["class_name"].toString();
    QString autoScore = QString::number(attempt["auto_score"].toDouble(), 'f', 1) + "%";
    int questionNumber = row + 1;

    QString info = QString("Grading Question %1 for %2\n"
                           "Quiz: %3\n"
                           "Course: %4\n"
                           "Class: %5\n"
                           "Auto-graded score for this attempt: %6\n\n"
                           "Enter the score for this question.")
                       .arg(questionNumber)
                       .arg(studentName)
                       .arg(quizTitle)
                       .arg(courseName)
                       .arg(className)
                       .arg(autoScore);

    m_scoreInfoLabel->setText(info);

    QString qaText = "Question:\n" + attempt["prompt"].toString() + "\n\n";
    qaText += "Student's Answer:\n" + attempt["student_response"].toString() + "\n\n";
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
    int questionId = m_attemptsTable->item(row, 1)->text().toInt();

    int ret = QMessageBox::question(this,
                                    "Submit Grade",
                                    QString("Submit grade of %1% for this question?")
                                        .arg(QString::number(m_scoreSpinBox->value(), 'f', 2)),
                                    QMessageBox::Yes | QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        QJsonObject data;
        data["attempt_id"] = attemptId;
        data["question_id"] = questionId;
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

    int questionCounter = 0;
    for (const QJsonValue &value : attempts) {
        QJsonObject attempt = value.toObject();
        int row = m_attemptsTable->rowCount();
        m_attemptsTable->insertRow(row);

        m_attemptsTable
            ->setItem(row, 0, new QTableWidgetItem(QString::number(attempt["attempt_id"].toInt())));
        m_attemptsTable->setItem(row,
                                 1,
                                 new QTableWidgetItem(
                                     QString::number(attempt["question_id"].toInt())));
        m_attemptsTable->setItem(row, 2, new QTableWidgetItem(attempt["student_name"].toString()));
        m_attemptsTable->setItem(row, 3, new QTableWidgetItem(attempt["quiz_title"].toString()));
        m_attemptsTable->setItem(row, 4, new QTableWidgetItem(attempt["course_name"].toString()));
        m_attemptsTable->setItem(row, 5, new QTableWidgetItem(attempt["class_name"].toString()));
        m_attemptsTable->setItem(row,
                                 6,
                                 new QTableWidgetItem(
                                     QString::number(attempt["attempt_number"].toInt())));

        // Show auto score
        double autoScore = attempt["auto_score"].toDouble();
        m_attemptsTable->setItem(row,
                                 7,
                                 new QTableWidgetItem(QString("%1%").arg(autoScore, 0, 'f', 1)));

        m_attemptsTable->setItem(row, 8, new QTableWidgetItem(QString::number(++questionCounter)));
        m_attemptsTable->setItem(row, 9, new QTableWidgetItem("Pending"));
    }
}
