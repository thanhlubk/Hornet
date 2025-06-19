#pragma once
#include "FancyWidgetsExport.h"
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include "FThemeableWidget.h"
#include "FThemeManager.h"
#include "FPopupMenu.h"

class FANCYWIDGETS_EXPORT FPanelSplitButton : public QWidget, public FThemeableWidget 
{
    Q_OBJECT
    DECLARE_THEME

public:
    explicit FPanelSplitButton(QWidget* parent = nullptr);

    void setIcon(const QString& icon, bool keep = false);
    void setText(const QString& text);
    void addWidget(QWidget* widget);
    
signals:
    void iconClicked();
    void textClicked();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    void updateStyle();
    void updateIcon();

    QLabel* m_pLabelIcon;
    QLabel* m_pLabelText;
    FPopupMenu* m_pMenu;

    QString m_strIcon;
    bool m_bKeep;
    enum HoverArea { None, Icon, Text } m_eHoverArea = None;
};