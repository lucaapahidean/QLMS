#ifndef ADMINDASHBOARD_H
#define ADMINDASHBOARD_H

#include "basedashboardwindow.h"

class AdminDashboard : public BaseDashboardWindow
{
    Q_OBJECT

public:
    explicit AdminDashboard(const QString &username, int userId, QWidget *parent = nullptr);
};

#endif // ADMINDASHBOARD_H
