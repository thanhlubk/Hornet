#pragma once
#include "FancyWidgetsExport.h"
#include <QPushButton>
#include <QPainter>
#include <QIcon>
#include "FThemeableWidget.h"

class FANCYWIDGETS_EXPORT FIconButton : public QPushButton, public FThemeableWidget
{
    Q_OBJECT
    DECLARE_THEME

public:
    explicit FIconButton(QWidget *parent = nullptr);
    explicit FIconButton(const QIcon &icon, QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private:
    bool m_bHover   = false;
    bool m_bPressed = false;
    bool m_bFocused = false;
};
