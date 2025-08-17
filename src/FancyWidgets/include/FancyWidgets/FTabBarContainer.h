#pragma once
#include "FancyWidgetsExport.h"
#include "FTabBar.h"
#include "FSplitter.h"
#include <QWidget>
#include <QSplitter>
#include <QHBoxLayout>

class FANCYWIDGETS_EXPORT FTabBarContainer : public QWidget, public FThemeableWidget
{
    Q_OBJECT
    DECLARE_THEME

public:
    explicit FTabBarContainer(QWidget *parent = nullptr);

    FTabBar* addTabBar(int index = -1);
    void addPane(unsigned short index, const QString &title, QWidget *widget);
    void addPane(const QString &title, QWidget *widget);

    void cleanupEmptyTabWidgets();

private slots:
    void closeTab(int index);
    void splitTab(FTabBarPane *fromBar, int index);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void updateStyleContainer(QWidget* widget);

    FSplitter* m_pSplitter;
};
