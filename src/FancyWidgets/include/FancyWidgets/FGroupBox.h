#pragma once
#include "FancyWidgetsExport.h"
#include <QGroupBox>
#include <QEvent>
#include "FThemeableWidget.h"
#include "FThemeManager.h"

class FANCYWIDGETS_EXPORT FGroupBox : public QGroupBox, public FThemeableWidget
{
    Q_OBJECT
    DECLARE_THEME

public:
    explicit FGroupBox(QWidget *parent = nullptr);
    explicit FGroupBox(const QString &title, QWidget *parent = nullptr);
    ~FGroupBox() override;

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void onFocusChanged(QWidget *old, QWidget *now);

private:
    bool m_bFocused = false;
};
