#pragma once
#include "FancyWidgetsExport.h"
#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QPixmap>
#include "FThemeableWidget.h"
#include "FThemeManager.h"

class FSearchBar : public QWidget, public FThemeableWidget
{
    Q_OBJECT
    DECLARE_THEME

public:
    explicit FSearchBar(QWidget *parent = nullptr);
    void setPlaceholderText(const QString &text);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QLabel* m_pLabelIcon;
    QLineEdit* m_pEditSearch;

    QHBoxLayout *m_pLayout;
};