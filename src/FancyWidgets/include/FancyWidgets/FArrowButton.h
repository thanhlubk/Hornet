#pragma once
#include "FancyWidgetsExport.h"
#include <QWidget>
#include <QLabel>
#include <QPropertyAnimation>
#include <QMargins>
#include "FThemeableWidget.h"
#include <QHBoxLayout>

class FANCYWIDGETS_EXPORT FArrowButton : public QWidget, public FThemeableWidget
{
    Q_OBJECT
    Q_PROPERTY(int rotation READ rotation WRITE setRotation)
    DECLARE_THEME

public:
    explicit FArrowButton(const QString& text, const QString& leftIcon, const QString& arrowIcon, bool keep, QWidget* parent = nullptr);

    int rotation() const { return m_iRotation; }
    void setRotation(int angle);
    bool isExpand() const;
    void enableMoveAnimation(bool enable);
    void enableRotateAnimation(bool enable);
    void enableBold(bool enable);

signals:
    void clicked();
    void arrowToggled(bool expanded);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void updateStyle();

    bool m_bHover;

private:
    void animateMove();
    void initSpacer();
    void initMainIcon();
    void initArrow();
    void updateStyleText();

    QWidget* m_pWidgetSpacer; 
    QLabel* m_pLabelMainIcon;
    QLabel* m_pLabelText;
    QLabel* m_pLabelArrow;
    QHBoxLayout* m_pLayout;
    QPropertyAnimation* m_pAnimRotate;
    QPropertyAnimation* m_pAnimMove;

    QString m_strIconArrow;
    QString m_strIconMain;
    QIcon m_iconArrow; // For loading only 1

    int m_iRotation;
    bool m_bExpanded;
    bool m_bAnimRotate;
    bool m_bAnimMove;
    bool m_bBold;
    bool m_bKeep;
};