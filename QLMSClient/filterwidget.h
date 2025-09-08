#ifndef FILTERWIDGET_H
#define FILTERWIDGET_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QComboBox;
class QLineEdit;
QT_END_NAMESPACE

class FilterWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FilterWidget(QWidget *parent = nullptr);

    void setFilterOptions(const QStringList &options);
    QString filterText() const;
    QString currentFilterOption() const;

signals:
    void filterChanged();

private:
    QComboBox *m_filterOptions;
    QLineEdit *m_filterEdit;
};

#endif // FILTERWIDGET_H
