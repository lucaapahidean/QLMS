#ifndef INSTRUCTORDASHBOARD_H
#define INSTRUCTORDASHBOARD_H

#include "basedashboardwindow.h"

class InstructorDashboard : public BaseDashboardWindow
{
    Q_OBJECT

public:
    explicit InstructorDashboard(const QString &username, int userId, QWidget *parent = nullptr);
};

#endif // INSTRUCTORDASHBOARD_H
