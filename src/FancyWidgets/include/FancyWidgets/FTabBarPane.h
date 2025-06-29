#pragma once
#include "FancyWidgetsExport.h"
#include <QTabBar>
#include <QDrag>
#include <QMimeData>
#include <QMouseEvent>
#include <QApplication>
#include "FThemeableWidget.h"

class FANCYWIDGETS_EXPORT FTabBarPane : public QTabBar, public FThemeableWidget
{
    Q_OBJECT
    DECLARE_THEME

public:
    explicit FTabBarPane(QWidget *parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    
signals:
    void tabDropped(FTabBarPane* fromBar, int index);

private:
    QPoint m_pointDragStartPos;
};
