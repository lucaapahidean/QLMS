#include "usermanagementwidget.h"
#include "filterwidget.h"
#include "networkmanager.h"
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

UserManagementWidget::UserManagementWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
    onRefreshClicked();
}

void UserManagementWidget::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);

    // Header
    auto *headerLayout = new QHBoxLayout();
    auto *titleLabel = new QLabel("User Management", this);
    QFont font = titleLabel->font();
    font.setPointSize(14);
    font.setBold(true);
    titleLabel->setFont(font);
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();

    m_refreshButton = new QPushButton("Refresh", this);
    headerLayout->addWidget(m_refreshButton);

    mainLayout->addLayout(headerLayout);

    // Filter
    m_filterWidget = new FilterWidget(this);
    m_filterWidget->setFilterOptions({"Username", "Role"});
    mainLayout->addWidget(m_filterWidget);

    // Table
    m_tableWidget = new QTableWidget(this);
    m_tableWidget->setColumnCount(3);
    m_tableWidget->setHorizontalHeaderLabels(QStringList() << "ID" << "Username" << "Role");
    m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableWidget->setAlternatingRowColors(true);
    m_tableWidget->setColumnHidden(0, true); // Hide the ID column
    mainLayout->addWidget(m_tableWidget);

    // Buttons
    auto *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_addButton = new QPushButton("Add User", this);
    buttonLayout->addWidget(m_addButton);

    m_deleteButton = new QPushButton("Delete User", this);
    m_deleteButton->setEnabled(false);
    buttonLayout->addWidget(m_deleteButton);

    mainLayout->addLayout(buttonLayout);

    // Connect signals
    connect(m_refreshButton, &QPushButton::clicked, this, &UserManagementWidget::onRefreshClicked);
    connect(m_addButton, &QPushButton::clicked, this, &UserManagementWidget::onAddUserClicked);
    connect(m_deleteButton, &QPushButton::clicked, this, &UserManagementWidget::onDeleteUserClicked);
    connect(m_tableWidget, &QTableWidget::itemSelectionChanged, this, [this]() {
        m_deleteButton->setEnabled(m_tableWidget->currentRow() >= 0);
    });
    connect(m_filterWidget, &FilterWidget::filterChanged, this, &UserManagementWidget::applyFilter);
}

void UserManagementWidget::onRefreshClicked()
{
    NetworkManager::instance().sendCommand("GET_ALL_USERS",
                                           QJsonObject(),
                                           [this](const QJsonObject &response) {
                                               handleUsersResponse(response);
                                           });
}

void UserManagementWidget::onAddUserClicked()
{
    QDialog dialog(this);
    dialog.setWindowTitle("Add New User");
    dialog.setModal(true);

    auto *layout = new QFormLayout(&dialog);

    auto *usernameEdit = new QLineEdit(&dialog);
    layout->addRow("Username:", usernameEdit);

    auto *passwordEdit = new QLineEdit(&dialog);
    passwordEdit->setEchoMode(QLineEdit::Password);
    layout->addRow("Password:", passwordEdit);

    auto *roleCombo = new QComboBox(&dialog);
    roleCombo->addItems(QStringList() << "student" << "instructor" << "admin");
    layout->addRow("Role:", roleCombo);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        if (usernameEdit->text().isEmpty() || passwordEdit->text().isEmpty()) {
            QMessageBox::warning(this, "Add User", "Username and password cannot be empty");
            return;
        }

        QJsonObject data;
        data["username"] = usernameEdit->text();
        data["password"] = passwordEdit->text();
        data["role"] = roleCombo->currentText();

        NetworkManager::instance()
            .sendCommand("CREATE_USER", data, [this](const QJsonObject &response) {
                if (response["type"].toString() == "OK") {
                    QMessageBox::information(this, "Success", "User created successfully");
                    onRefreshClicked();
                } else {
                    QMessageBox::critical(this, "Error", response["message"].toString());
                }
            });
    }
}

void UserManagementWidget::onDeleteUserClicked()
{
    int row = m_tableWidget->currentRow();
    if (row < 0)
        return;

    int userId = m_tableWidget->item(row, 0)->text().toInt();
    QString username = m_tableWidget->item(row, 1)->text();

    int ret
        = QMessageBox::question(this,
                                "Delete User",
                                QString("Are you sure you want to delete user '%1'?").arg(username),
                                QMessageBox::Yes | QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        QJsonObject data;
        data["user_id"] = userId;

        NetworkManager::instance()
            .sendCommand("DELETE_USER", data, [this](const QJsonObject &response) {
                if (response["type"].toString() == "OK") {
                    QMessageBox::information(this, "Success", "User deleted successfully");
                    onRefreshClicked();
                } else {
                    QMessageBox::critical(this, "Error", response["message"].toString());
                }
            });
    }
}

void UserManagementWidget::handleUsersResponse(const QJsonObject &response)
{
    if (response["type"].toString() == "DATA_RESPONSE") {
        populateTable(response["data"].toArray());
    } else {
        QMessageBox::critical(this, "Error", "Failed to fetch users");
    }
}

void UserManagementWidget::populateTable(const QJsonArray &users)
{
    m_tableWidget->setRowCount(0);

    for (const QJsonValue &value : users) {
        QJsonObject user = value.toObject();
        int row = m_tableWidget->rowCount();
        m_tableWidget->insertRow(row);

        m_tableWidget->setItem(row,
                               0,
                               new QTableWidgetItem(QString::number(user["user_id"].toInt())));
        m_tableWidget->setItem(row, 1, new QTableWidgetItem(user["username"].toString()));
        m_tableWidget->setItem(row, 2, new QTableWidgetItem(user["role"].toString()));
    }

    // Resize all columns except the last one to their contents
    for (int i = 0; i < m_tableWidget->columnCount() - 1; ++i) {
        m_tableWidget->resizeColumnToContents(i);
    }
    // Ensure the last column stretches to fill the remaining space
    m_tableWidget->horizontalHeader()->setStretchLastSection(true);
}

void UserManagementWidget::applyFilter()
{
    QString filterText = m_filterWidget->filterText();
    QString filterColumn = m_filterWidget->currentFilterOption();
    int column = (filterColumn == "Username") ? 1 : 2;

    for (int i = 0; i < m_tableWidget->rowCount(); ++i) {
        bool match = m_tableWidget->item(i, column)->text().contains(filterText,
                                                                     Qt::CaseInsensitive);
        m_tableWidget->setRowHidden(i, !match);
    }
}
