#ifndef COURSEMANAGEMENTWIDGET_H
#define COURSEMANAGEMENTWIDGET_H

#include <QJsonArray>
#include <QJsonObject>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QListWidget;
class QTextEdit;
class QPushButton;
class QGroupBox;
class QVBoxLayout;
QT_END_NAMESPACE

class CourseManagementWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CourseManagementWidget(QWidget *parent = nullptr);

private slots:
    void onRefreshMaterials();
    void onMaterialSelected();
    void onDeleteMaterial();
    void onAddMaterial();
    void handleMaterialsResponse(const QJsonObject &response);
    void handleMaterialDetailsResponse(const QJsonObject &response);
    void handleDeleteMaterialResponse(const QJsonObject &response);

private:
    void setupUi();
    void populateMaterialsList(const QJsonArray &materials);
    void displayLesson(const QJsonObject &lesson);
    void displayQuiz(const QJsonObject &quiz);
    void clearContentArea();

    QJsonArray m_materials;

    // UI Elements
    QListWidget *m_materialsListWidget;
    QPushButton *m_refreshButton;
    QPushButton *m_addButton;
    QPushButton *m_deleteButton;
    QGroupBox *m_contentGroup;
    QVBoxLayout *m_contentLayout;
    QTextEdit *m_contentView;
};

#endif // COURSEMANAGEMENTWIDGET_H
