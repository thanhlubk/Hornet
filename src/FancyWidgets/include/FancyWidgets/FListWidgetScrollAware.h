#pragma once
#include "FancyWidgetsExport.h"
#include "FThemeableWidget.h"
#include "FThemeManager.h"
#include "FListWidget.h"

class FANCYWIDGETS_EXPORT FListWidgetScrollAware : public FListWidget, public FThemeableWidget
{
    Q_OBJECT
    DECLARE_THEME

public:
    explicit FListWidgetScrollAware(QWidget *parent = nullptr);

    void reserveScrollBarSpace(int reserveSpace);

protected:
    QMargins adjustViewportMargins() override;
    // void updateViewPort() override;

private slots:
    void updateScrollAware(int min, int max);

private:
    bool m_bScrollAware = false;
    int m_iReserveSpace;
};