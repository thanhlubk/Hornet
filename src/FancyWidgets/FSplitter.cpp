#include <FancyWidgets/FSplitter.h>

FSplitter::FSplitter(QWidget *parent)
    : FSplitter(Qt::Horizontal, parent)
{
}

FSplitter::FSplitter(Qt::Orientation orientation, QWidget *parent)
    : QSplitter(orientation, parent), FThemeableWidget(), m_iHandleSpacing(0)
{
    SET_UP_THEME(FSplitter)
}

void FSplitter::applyTheme()
{
    setStyleSheet(QString("background: %1;").arg(getColorTheme(0).name(QColor::HexArgb)));
    setHandleWidth(getDisplaySize(0).toInt());

    auto color2 = getColorTheme(1).name();
    setHandleSeparatorColor(QColor(getColorTheme(1).name()));
}

QColor FSplitter::getColorTheme(int idx)
{
    SET_UP_COLOR(0, Default, transparent);
    SET_UP_COLOR(1, Default, transparent);
    SET_UP_COLOR(2, Custom1, transparent);

    SET_UP_COLOR(0, Custom1, transparent);
    SET_UP_COLOR(1, Custom1, textNormal);
    SET_UP_COLOR(2, Custom1, backgroundSection1);
    
    return QColor(qRgb(0, 0, 0));
}

QVariant FSplitter::getDisplaySize(int idx)
{
    SET_UP_DISPLAY_SIZE(0, Primary, offsetSize1);

    return QVariant(0);
}

void FSplitter::setHandleSeparatorColor(QColor color)
{
    m_colorSeparator = color;
    updateHandleSeparatorColor();
}

void FSplitter::setHandleSeparatorOffset(int offsetStart, int offsetEnd)
{
    m_arrOffset[0] = offsetStart;
    m_arrOffset[1] = offsetEnd;
    updateHandleSeparatorOffset();
}

void FSplitter::setHandleWidth(int width)
{
    QSplitter::setHandleWidth(width + 2 * m_iHandleSpacing);
    updateHandleSeparatorWidth();
}

void FSplitter::setHandleSpacing(int spacing)
{
    if (spacing < 0)
        spacing = 0; // ensure non-negative spacing
    m_iHandleSpacing = spacing;
    QSplitter::setHandleWidth(handleWidth() + 2 * m_iHandleSpacing);
    updateHandleSeparatorWidth();
}

QSplitterHandle *FSplitter::createHandle()
{
    auto handle = new FSplitterHandle(orientation(), this);
    m_listHandle.append(handle); // track for later

    connect(handle, &QObject::destroyed, this, [this, handle]() { m_listHandle.removeAll(handle); });

    updateHandleSeparatorColor();
    updateHandleSeparatorOffset();
    updateHandleSeparatorWidth();

    return handle;
}

void FSplitter::updateHandleSeparatorColor()
{
    for (auto pHandle : m_listHandle)
    {
        auto pFHandle = qobject_cast<FSplitterHandle *>(pHandle);
        if (pFHandle)
            pFHandle->separator()->setColorLine(m_colorSeparator);
    }
}

void FSplitter::updateHandleSeparatorOffset()
{
    for (auto pHandle : m_listHandle)
    {
        auto pFHandle = qobject_cast<FSplitterHandle *>(pHandle);
        if (!pFHandle)
            continue;
        pFHandle->separator()->setOffset(m_arrOffset[0], m_arrOffset[1]);
    }
}

void FSplitter::updateHandleSeparatorWidth()
{
    for (auto pHandle : m_listHandle)
    {
        auto pFHandle = qobject_cast<FSplitterHandle *>(pHandle);
        if (!pFHandle)
            continue;

        if (handleWidth() == 1)
            pFHandle->separator()->setLinePosition(0);
        else
            pFHandle->separator()->setLinePosition(handleWidth()/2);
        pFHandle->separator()->setLineWidth(handleWidth() - 2 * m_iHandleSpacing);
    }
}