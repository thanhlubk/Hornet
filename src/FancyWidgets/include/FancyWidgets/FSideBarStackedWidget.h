#pragma once
#include "FancyWidgetsExport.h"
#include <QStackedWidget>
#include <QMap>
#include <QWidget>
#include <QSet>
#include <QFrame>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QMouseEvent>
#include <QListWidget>
#include "FThemeableWidget.h"
#include "FThemeManager.h"
#include <QJsonArray>

class FSideBarStackedFilletFrame : public QFrame {
    Q_OBJECT
public:
    using QFrame::QFrame;

    void enablePaint(bool enable) { bPaint = enable; };
    void setAbove(bool enable) { bAbove = enable; };
    void setFilletPosition(int position) 
    { 
        if (position < 0)
            position = 0;
        iPosition = position; 
    };
    void setBrushColor(const QColor& color) { brushColor = color; update(); };

private:
    bool bPaint = false;
    bool bAbove = false;
    int iPosition = 0;
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
                QPoint corner(width() + 10, 0); // Corner of frame
                QPoint corner2(0, height() + 10); // Corner of frame
                QPoint corner3(width() + 10, height() + 10); // Corner of frame


                path.moveTo(corner); // Start at point
                if (iPosition == 0)
                    path.lineTo(QPoint(0, 0));
                else
                    path.arcTo(0, 0, iPosition*2, iPosition*2, 90, 90); //  Quarter-circle arc
                path.lineTo(corner2);
                path.lineTo(corner3);
                path.lineTo(corner); // Line to end point
            }
            else
            {
                QPoint corner(0, -10); // Corner of frame
                QPoint corner2(width() + 10, height());
                QPoint corner3(width() + 10, -10); // Corner of frame
                
                path.moveTo(corner); // Start at corner
                if (iPosition == 0)
                    path.lineTo(QPoint(0, height()));
                else
                    path.arcTo(0, height()-iPosition*2, iPosition*2, iPosition*2, 180, 90); //  Quarter-circle arc
                path.lineTo(corner2);
                path.lineTo(corner3);
                path.lineTo(corner); // Line to corner
            }
            painter.setBrush(brushColor); // Fill the shape with red
            painter.setPen(Qt::NoPen); // Remove outline
            painter.drawPath(path);
        }
    }
};

class FANCYWIDGETS_EXPORT FSideBarStackedWidget : public QStackedWidget, public FThemeableWidget
{
    Q_OBJECT
    DECLARE_THEME

public:
    using PageFactory = std::function<QWidget*()>;

    explicit FSideBarStackedWidget(QWidget* parent = nullptr);

    void registerPage(int index, PageFactory factory);
    int registerPage(PageFactory factory);
    void setCurrentLazyIndex(int index);

    bool isCollapsed() const;   
    void setCollapsed(bool collapse);
    int getCollapseWidth();
    QWidget* addPageJson(const QJsonArray& jsonArray, const QString& title);
    QWidget* addPage(PageFactory factory, const QString& title);

signals:
    void collapseChanged();

public slots:
    void updateFillet(int top, int bottom, bool pin);

protected:
    void resizeEvent(QResizeEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    
    void addSplitButton(const QJsonObject& obj, QWidget* parent);
    void addPanelButton(const QJsonObject& obj, QWidget* parent);
    void addExpandableButton(const QJsonObject &obj, QWidget *parent);
    void addArrowButton(const QJsonObject &obj, QWidget *parent);
    QWidget* createPageLayout(const QString &title, QWidget *widget);

private:
    void updateCollapseIcon();
    void updateStyleCurrentPage();
    void enableMouseTrackingRecursive(QWidget* widget);
    bool isNearRightEdge(const QPoint& pos) const;

    std::unordered_map<int, PageFactory> m_mapPageFactories;
    std::unordered_map<int, int> m_mapItemPageIdx;

    QSet<int> m_setInitializedPages;
    QPoint m_pointStartPos;

    FSideBarStackedFilletFrame* m_pFrameTop;
    FSideBarStackedFilletFrame* m_pFrameBottom;
    QPushButton* m_pCollapseButton;

    bool m_bCollapsed;
    bool m_bResizing;
    int m_iStartWidth;
    int m_iCollapseWidth;
};