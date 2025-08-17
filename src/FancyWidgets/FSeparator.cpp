#include "FancyWidgets/FSeparator.h"
#include <QPainter>
#include <QPen>

FSeparator::FSeparator(Qt::Orientation orientation, QWidget *parent)
    : QWidget(parent), m_colorLine(QColor()), m_eOrientation(orientation), m_colorBackground(QColor()), m_fLineWidth(1.0), m_iLinePosition(0), m_arrOffset({0, 0})
{
}

void FSeparator::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, m_colorBackground);
    setAutoFillBackground(true);
    setPalette(pal);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Line appearance
    QPen pen;
    pen.setColor(m_colorLine);
    pen.setWidthF(m_fLineWidth); // Thickness of the line
    pen.setCapStyle(Qt::RoundCap); // Rounded ends (fillet)
    painter.setPen(pen);

    if (m_eOrientation == Qt::Horizontal) {
        // Draw horizontal line
        painter.drawLine(QPointF(m_arrOffset[0] + m_fLineWidth / 2, m_iLinePosition), QPointF(width() - m_arrOffset[1] - m_fLineWidth / 2, m_iLinePosition));
    } else if (m_eOrientation == Qt::Vertical) {
        // Draw vertical line
        painter.drawLine(QPointF(m_iLinePosition, m_arrOffset[0] + m_fLineWidth / 2), QPointF(m_iLinePosition, height() - m_arrOffset[1] - m_fLineWidth / 2));
    }
}
void FSeparator::setColorLine(const QColor &color)
{
    m_colorLine = color;
}

QColor FSeparator::colorLine() const
{
    return m_colorLine;
}

void FSeparator::setColorBackground(const QColor &color)
{
    m_colorBackground = color;
}

QColor FSeparator::colorBackground() const
{
    return m_colorBackground;
}

void FSeparator::setLineWidth(float width)
{
    m_fLineWidth = width;
}

float FSeparator::lineWidth() const
{
    return m_fLineWidth;
}
void FSeparator::setLinePosition(int pos)
{
    m_iLinePosition = pos;
}

int FSeparator::linePosition() const
{
    return m_iLinePosition;
}
void FSeparator::setOffsetLeft(int offset)
{
    if (m_eOrientation == Qt::Horizontal)
        m_arrOffset[0] = offset;
}

int FSeparator::offsetLeft() const
{
    if (m_eOrientation == Qt::Horizontal)
        return m_arrOffset[0];
    return 0;
}

void FSeparator::setOffsetRight(int offset)
{
    if (m_eOrientation == Qt::Horizontal)
        m_arrOffset[1] = offset;
}

int FSeparator::offsetRight() const
{
    if (m_eOrientation == Qt::Horizontal)
        return m_arrOffset[1];
    return 0;
}

void FSeparator::setOffsetTop(int offset)
{
    if (m_eOrientation == Qt::Vertical)
        m_arrOffset[0] = offset;
}

int FSeparator::offsetTop() const
{
    if (m_eOrientation == Qt::Vertical)
        return m_arrOffset[0];
    return 0;
}

void FSeparator::setOffsetBottom(int offset)
{
    if (m_eOrientation == Qt::Vertical)
        m_arrOffset[1] = offset;
}

int FSeparator::offsetBottom() const
{
    if (m_eOrientation == Qt::Vertical)
        return m_arrOffset[1];
    return 0;
}

void FSeparator::setOffset(int offsetStart, int offsetEnd)
{
    m_arrOffset[0] = offsetStart;
    m_arrOffset[1] = offsetEnd;
}