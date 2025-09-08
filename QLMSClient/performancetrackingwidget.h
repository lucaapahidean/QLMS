#ifndef PERFORMANCETRACKINGWIDGET_H
#define PERFORMANCETRACKINGWIDGET_H

#include <QJsonArray>
#include <QJsonObject>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QTreeWidget;
class QTreeWidgetItem;
class QTextEdit;
class QPushButton;
class QGroupBox;
QT_END_NAMESPACE

class FilterWidget;

class PerformanceTrackingWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PerformanceTrackingWidget(int instructorId, QWidget *parent = nullptr);

private slots:
    void onRefresh();
    void onItemSelected();
    void handleClassesResponse(const QJsonObject &response);
    void handleCoursesResponse(const QJsonObject &response, QTreeWidgetItem *classItem);
    void handleMaterialsResponse(const QJsonObject &response, QTreeWidgetItem *courseItem);
    void handleAttemptDetailsResponse(const QJsonObject &response);
    void applyFilter();

private:
    void setupUi();
    void displayAttemptDetails(const QJsonObject &attemptData);
    void clearDetailsArea();
    bool applyFilterRecursive(QTreeWidgetItem *item, const QString &text, const QString &filterBy);

    int m_instructorId;

    // UI Elements
    FilterWidget *m_filterWidget;
    QTreeWidget *m_treeWidget;
    QPushButton *m_refreshButton;
    QGroupBox *m_detailsGroup;
    QTextEdit *m_detailsTextEdit;
};

#endif // PERFORMANCETRACKINGWIDGET_H
