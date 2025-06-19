#pragma once
#include "FancyWidgetsExport.h"
#include <QWidget>
#include <QScrollArea>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFrame>
#include <QPainter>
#include <QPainterPath>
#include "FThemeableWidget.h"
#include "FThemeManager.h"

class FSideBarTabFilletFrame : public QFrame {
    Q_OBJECT
    
public:
    FSideBarTabFilletFrame(QWidget* parent) : QFrame(parent) {}
    void enablePaint(bool enable) { bPaint = enable; };
    void setAbove(bool enable) { bAbove = enable; };
    void setBrushColor(const QColor& color) { brushColor = color; update(); };
    
private:
    bool bPaint = false;
    bool bAbove = false;
    QColor brushColor;

protected:
    void paintEvent(QPaintEvent *event) override {
        QFrame::paintEvent(event);

        if (bPaint)
        {
            QPainter painter(this);
            painter.setRenderHint(QPainter::Antialiasing);

            QPainterPath path;
            if (bAbove)
            {
                QPoint corner(width(), height() + 10); // Corner of frame
                QPoint point(width() - height()*2, height() + 10); // Point offset from corner

                path.moveTo(point); // Start at point
                path.arcTo(width() - height()*2, -height(), height()*2, height()*2, 270, 90); //  Quarter-circle arc
                path.lineTo(corner);
                path.lineTo(point); // Line to end point
            }
            else
            {
                QPoint corner(width(), -10); // Corner of frame
                QPoint point(width() - height()*2, -10);
                
                path.moveTo(corner); // Start at corner
                path.arcTo(width() - height()*2, 0, height()*2, height()*2, 0, 90); //  Quarter-circle arc
                path.lineTo(point);
                path.lineTo(corner); // Line to corner
            }
            painter.setBrush(brushColor); // Fill the shape with red
            painter.setPen(Qt::NoPen); // Remove outline
            painter.drawPath(path);
        }
    }
};

class FANCYWIDGETS_EXPORT FSideBarScrollArea : public QWidget, public FThemeableWidget
{
    Q_OBJECT
    DECLARE_THEME

public:
    explicit FSideBarScrollArea(QWidget* parent = nullptr);

    void addItem(QWidget* item);
    void addPinItem(QWidget* item);
    void resizeEvent(QResizeEvent* event);
    void uncheckAllTab();
    void setScrollIcon(const QString& icon, bool keep=false);

signals:
    void scrollingPositionChanged(int top, int bottom, bool pin);

protected:
    void paintEvent(QPaintEvent* event) override;
    void updateCurrentTabPosition();
    void updateTabView(QWidget* currentTab);
    void updateFrameColor();
    void updateScrollIcon();

private slots:
    void scrollUp();
    void scrollDown();
    void updateScrollButtonVisibility();
    void clickTab();

private:
    QVBoxLayout* m_pMainLayout;
    QPushButton* m_pScrollUpButton;
    QPushButton* m_pScrollDownButton;
    QScrollArea* m_pScrollArea;
    QWidget* m_pScrollContent;
    QVBoxLayout* m_pContentLayout;
    QWidget* m_pCurrentWidget;

    QVector<QWidget*> m_vePinnedButton;
    QVector<FSideBarTabFilletFrame*> m_veFrame;
    QVector<FSideBarTabFilletFrame*> m_vePinnedFrame;
    QString m_strIconScroll;
    
    bool m_bKeep;
};