#pragma once
#include "FancyWidgetsExport.h"
#include <QTabWidget>
#include "FTabBarPane.h"
#include <QPushButton>

class FANCYWIDGETS_EXPORT FTabBar : public QTabWidget, public FThemeableWidget
{
    Q_OBJECT
    DECLARE_THEME
    
public:
    FTabBar(QWidget *parent = nullptr);
    

signals:
    void tabRemoved();
    void tabDropped(FTabBarPane *fromBar, int index);

private slots:
    void recieveTabDropped(FTabBarPane *fromBar, int index);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void updateCloseIcon();

    QPushButton* m_pCloseButton;
};
