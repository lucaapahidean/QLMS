#include "filterwidget.h"
#include <QComboBox>
#include <QHBoxLayout>
#include <QLineEdit>

FilterWidget::FilterWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_filterOptions = new QComboBox(this);
    layout->addWidget(m_filterOptions);

    m_filterEdit = new QLineEdit(this);
    m_filterEdit->setPlaceholderText("Enter filter text...");
    layout->addWidget(m_filterEdit);

    connect(m_filterEdit, &QLineEdit::textChanged, this, &FilterWidget::filterChanged);
    connect(m_filterOptions, &QComboBox::currentIndexChanged, this, &FilterWidget::filterChanged);
}

void FilterWidget::setFilterOptions(const QStringList &options)
{
    m_filterOptions->clear();
    m_filterOptions->addItems(options);
}

QString FilterWidget::filterText() const
{
    return m_filterEdit->text();
}

QString FilterWidget::currentFilterOption() const
{
    return m_filterOptions->currentText();
}
