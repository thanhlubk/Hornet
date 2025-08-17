#pragma once
#include "FancyWidgetsExport.h"
#include "FThemeableWidget.h"
#include <QWidget>
#include "FCheckBox.h"
#include <QHBoxLayout>
#include <QLabel>

class FANCYWIDGETS_EXPORT FTreeItem : public QWidget, public FThemeableWidget
{
    Q_OBJECT
    DECLARE_THEME

public:
    FTreeItem(const QString &title, const QString &icon, QWidget *parent = nullptr);
    void enableBold(bool enable);

signals:
    void itemClicked();
    void itemDoubleClicked();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    
private:
    void initMainIcon();
    void updateStyleText();
    void updateStyle();

    QLabel* m_pLabelMainIcon;
    QLabel* m_pLabelText;
    FCheckBox* m_pCheckBox;
    QHBoxLayout* m_pLayout;

    QString m_strIconMain;
    bool m_bKeep;
    bool m_bHover;
    bool m_bBold;
};