#include <FancyWidgets/FListWidgetScrollAware.h>
#include <QScrollBar>
#include <FancyWidgets/FUtil.h>

FListWidgetScrollAware::FListWidgetScrollAware(QWidget *parent)
    : FListWidget(parent), FThemeableWidget(), m_iReserveSpace(0), m_bScrollAware(false)
{
    setViewMode(QListView::ListMode); // List mode
    setFlow(QListView::TopToBottom);  // Flow left to right
    setWrapping(false);               // Wrap into rows
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    connect(this->verticalScrollBar(), &QScrollBar::rangeChanged, this, &FListWidgetScrollAware::updateScrollAware);

    SET_UP_THEME(FListWidgetScrollAware)
}

void FListWidgetScrollAware::applyTheme()
{
    reserveScrollBarSpace(getDisplaySize(1).toInt() + getDisplaySize(0).toInt() / 2);

    auto strScrollbarVerticalStyle = FUtil::getStyleScrollbarVertical2(getDisplaySize(1).toInt(), getDisplaySize(0).toInt() / 2, getDisplaySize(0).toInt() / 2, 0, ":/FancyWidgets/res/icon/small/up.png", ":/FancyWidgets/res/icon/small/down.png", getColorTheme(1).name(), getColorTheme(1).name());

    auto strScrollbarHorizontalStyle = FUtil::getStyleScrollbarHorizontal1(getDisplaySize(1).toInt(), 0, 0, 0, getColorTheme(1).name(), getColorTheme(1).name());

    setStyleSheet(QString("QListWidget { background: %1; padding-left: 0px; padding-right: 0px; padding-top: 0px; padding-bottom: 0px;} QListWidget::item { outline: none; border: none; }").arg(getColorTheme(0).name(QColor::HexArgb)) + strScrollbarVerticalStyle + strScrollbarHorizontalStyle);
}

QColor FListWidgetScrollAware::getColorTheme(int idx)
{
    SET_UP_COLOR(0, Default, transparent);
    SET_UP_COLOR(1, Default, textNormal);

    return QColor(qRgb(0, 0, 0));
}

QVariant FListWidgetScrollAware::getDisplaySize(int idx)
{
    SET_UP_DISPLAY_SIZE(0, Primary, offsetSize2);
    SET_UP_DISPLAY_SIZE(1, Primary, scrollbarWidth);
    return QVariant(0); // Default font size
}

void FListWidgetScrollAware::reserveScrollBarSpace(int reserveSpace)
{
    m_iReserveSpace = reserveSpace;
    updateScrollAware(0, 0);
}

QMargins FListWidgetScrollAware::adjustViewportMargins()
{
    auto margins = FListWidget::adjustViewportMargins();
    if (m_bScrollAware)
        margins.setRight(m_iReserveSpace + margins.right());

    return margins;
}

#if 0
void FListWidgetScrollAware::updateViewPort()
{
    // FListWidget::updateViewPort();
    // auto viewMargins = viewportMargins();
    // if (m_bScrollAware)
    //     // If scroll aware, reserve space for the scrollbar
    //     setViewportMargins(viewMargins.left(), viewMargins.top(), viewMargins.right() + m_iReserveSpace, viewMargins.bottom());
    // else
    //     setViewportMargins(viewMargins);

    int borderSpacing = 0;
    if (!isSpacingBorderEnabled())
    {
        borderSpacing = spacing();
    }

    if (m_bScrollAware)
    {
        setViewportMargins(-borderSpacing, 0, m_iReserveSpace - borderSpacing, 0);
    }
    else
    {
        setViewportMargins(-borderSpacing, 0, -borderSpacing, 0);
    }
}
#endif

void FListWidgetScrollAware::updateScrollAware(int min, int max)
{
    // if (max > 0)
    //     this->setViewportMargins(0, 0, 0, 0);
    // else
    //     this->setViewportMargins(0, 0, m_iReserveSpace, 0);

    if (max > 0)
        m_bScrollAware = false;
    else
        m_bScrollAware = true;

    updateViewPort();
}