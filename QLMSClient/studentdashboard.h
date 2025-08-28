#ifndef STUDENTDASHBOARD_H
#define STUDENTDASHBOARD_H

#include "basedashboardwindow.h"

class StudentDashboard : public BaseDashboardWindow
{
    Q_OBJECT

public:
    explicit StudentDashboard(const QString &username, int userId, QWidget *parent = nullptr);
};

#endif // STUDENTDASHBOARD_H
