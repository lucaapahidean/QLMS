#include "usersearchdialog.h"
#include "networkmanager.h"
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QListView>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QStringListModel>
#include <QVBoxLayout>

UserSearchDialog::UserSearchDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi();
    fetchUsers();
}

void UserSearchDialog::setupUi()
{
    setWindowTitle("Assign User");
    setMinimumWidth(300);

    auto *layout = new QVBoxLayout(this);
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("Search by username...");
    layout->addWidget(m_searchEdit);

    m_userListView = new QListView(this);
    m_userModel = new QStringListModel(this);
    m_userListView->setModel(m_userModel);
    layout->addWidget(m_userListView);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_okButton = buttonBox->button(QDialogButtonBox::Ok);
    m_okButton->setEnabled(false);
    layout->addWidget(buttonBox);

    connect(m_searchEdit, &QLineEdit::textChanged, this, &UserSearchDialog::filterUsers);
    connect(m_userListView->selectionModel(),
            &QItemSelectionModel::selectionChanged,
            this,
            &UserSearchDialog::onUserSelected);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void UserSearchDialog::fetchUsers()
{
    NetworkManager::instance()
        .sendCommand("GET_ALL_USERS", QJsonObject(), [this](const QJsonObject &response) {
            if (response["type"].toString() == "DATA_RESPONSE") {
                m_allUsers = response["data"].toArray();
                filterUsers(""); // Populate with all users initially
            } else {
                QMessageBox::critical(this, "Error", "Failed to fetch user list.");
            }
        });
}

void UserSearchDialog::filterUsers(const QString &text)
{
    QStringList userList;
    for (const QJsonValue &value : m_allUsers) {
        QJsonObject user = value.toObject();
        QString username = user["username"].toString();
        if (username.contains(text, Qt::CaseInsensitive)) {
            userList.append(QString("%1 (ID: %2, Role: %3)")
                                .arg(username)
                                .arg(user["user_id"].toInt())
                                .arg(user["role"].toString()));
        }
    }
    m_userModel->setStringList(userList);
    m_userListView->clearSelection();
    m_okButton->setEnabled(false);
    m_selectedUserId = -1;
}

void UserSearchDialog::onUserSelected()
{
    QModelIndexList selectedIndexes = m_userListView->selectionModel()->selectedIndexes();
    if (selectedIndexes.isEmpty()) {
        m_okButton->setEnabled(false);
        m_selectedUserId = -1;
        return;
    }

    QString selectedText = selectedIndexes.first().data().toString();
    // Extract user ID from the string "username (ID: 123, Role: student)"
    QRegularExpression re("\\(ID: (\\d+),");
    QRegularExpressionMatch match = re.match(selectedText);
    if (match.hasMatch()) {
        m_selectedUserId = match.captured(1).toInt();
        m_okButton->setEnabled(true);
    } else {
        m_selectedUserId = -1;
        m_okButton->setEnabled(false);
    }
}

int UserSearchDialog::selectedUserId() const
{
    return m_selectedUserId;
}
