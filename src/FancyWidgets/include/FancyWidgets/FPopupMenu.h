#pragma once
#include "FancyWidgetsExport.h"
#include <QWidget>
#include <QVBoxLayout>
#include <QResizeEvent>
#include "FThemeableWidget.h"
#include "FThemeManager.h"

class FPopupMenu : public QWidget, public FThemeableWidget 
{
    Q_OBJECT
    DECLARE_THEME

public:
    explicit FPopupMenu(QWidget* parent = nullptr);
    
    enum PopupPosition
    {
        Bottom,
        Right,
    };

    void setPopupPosition(PopupPosition pos);
    void addWidget(QWidget* widget);

signals:
    void popupHidden();

public slots:
    void appear();

protected:
    void hideEvent(QHideEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    QVBoxLayout* m_pLayout;
    PopupPosition m_ePosition;
};
