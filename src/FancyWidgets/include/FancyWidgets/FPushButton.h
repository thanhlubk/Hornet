#pragma once
#include "FancyWidgetsExport.h"
#include <QPushButton>
#include <QPainter>
#include "FThemeableWidget.h"

class FANCYWIDGETS_EXPORT FPushButton : public QPushButton, public FThemeableWidget
{
    Q_OBJECT
    DECLARE_THEME

public:
    explicit FPushButton(QWidget *parent = nullptr);
    explicit FPushButton(const QString &text, QWidget *parent = nullptr);

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
