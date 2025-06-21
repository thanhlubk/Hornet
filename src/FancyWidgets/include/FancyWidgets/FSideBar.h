#pragma once
#include "FancyWidgetsExport.h"
#include <QWidget>
#include <QPointer>
#include <QList>
#include <QVBoxLayout>
#include "FSideBarScrollArea.h"
#include "FSideBarStackedWidget.h"
#include "FSideBarTab.h"
#include "FThemeableWidget.h"
#include "FThemeManager.h"

class FANCYWIDGETS_EXPORT FSideBar : public QWidget, public FThemeableWidget 
{
    Q_OBJECT
    DECLARE_THEME

public:
    explicit FSideBar(QWidget* parent = nullptr);

    void observeWidget(QWidget* child);
    // void setScrollIcon(const QString& icon, bool bKeep=false);
    // void setCollapseIcon(const QString& icon, bool bKeep=false);

    void readJson(const QString &jsonFile);
    void createTabSetting(const QString& icon1, const QString& icon2);
    void createTabOutline(const QString& icon1, const QString& icon2);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    // QWidget* createPage(const QString& xml);

public slots:
    void toggleCollapse();
    void updateContainerWidth();

private:
    QHBoxLayout* m_pLayout;
    FSideBarScrollArea* m_pScrollArea;
    FSideBarStackedWidget* m_pStackedWidget;

    QList<QPointer<QWidget>> m_listObservedChild;
};