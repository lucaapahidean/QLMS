#ifndef USERSEARCHDIALOG_H
#define USERSEARCHDIALOG_H

#include <QDialog>
#include <QJsonArray>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QListView;
class QStringListModel;
class QPushButton;
QT_END_NAMESPACE

class UserSearchDialog : public QDialog
{
    Q_OBJECT

public:
    explicit UserSearchDialog(QWidget *parent = nullptr);
    int selectedUserId() const;

private slots:
    void filterUsers(const QString &text);
    void onUserSelected();

private:
    void setupUi();
    void fetchUsers();

    QLineEdit *m_searchEdit;
    QListView *m_userListView;
    QPushButton *m_okButton;

    QJsonArray m_allUsers;
    QStringListModel *m_userModel;
    int m_selectedUserId = -1;
};

#endif // USERSEARCHDIALOG_H
