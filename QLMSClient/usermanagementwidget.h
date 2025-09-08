#ifndef USERMANAGEMENTWIDGET_H
#define USERMANAGEMENTWIDGET_H

#include <QJsonArray>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QTableWidget;
class QPushButton;
QT_END_NAMESPACE

class FilterWidget;

class UserManagementWidget : public QWidget
{
    Q_OBJECT

public:
    explicit UserManagementWidget(QWidget *parent = nullptr);

private slots:
    void onRefreshClicked();
    void onAddUserClicked();
    void onDeleteUserClicked();
    void handleUsersResponse(const QJsonObject &response);
    void applyFilter();

private:
    void setupUi();
    void populateTable(const QJsonArray &users);

    FilterWidget *m_filterWidget;
    QTableWidget *m_tableWidget;
    QPushButton *m_refreshButton;
    QPushButton *m_addButton;
    QPushButton *m_deleteButton;
};

#endif // USERMANAGEMENTWIDGET_H
