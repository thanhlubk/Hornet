#include "FancyWidgets/FHorizontalSeparator.h"
#include <QPainter>
#include <QPen>

FHorizontalSeparator::FHorizontalSeparator(QWidget* parent)
    : QWidget(parent), m_colorLine(QColor()), m_colorBackground(QColor()), m_fLineWidth(1.0), m_iLinePosition(0), m_iOffsetLeft(0), m_iOffsetRight(0)
{
}

void FHorizontalSeparator::paintEvent(QPaintEvent* event)
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
    pen.setWidthF(m_fLineWidth);                   // Thickness of the line
    pen.setCapStyle(Qt::RoundCap);     // Rounded ends (fillet)
    painter.setPen(pen);

    painter.drawLine(QPointF(m_iOffsetLeft, m_iLinePosition), QPointF(width() - m_iOffsetRight, m_iLinePosition));
}
void FHorizontalSeparator::setColorLine(const QColor &color)
{
    m_colorLine = color;
}

QColor FHorizontalSeparator::colorLine() const
{
    return m_colorLine;
}

void FHorizontalSeparator::setColorBackground(const QColor &color)
{
    m_colorBackground = color;
}

QColor FHorizontalSeparator::colorBackground() const
{
    return m_colorBackground;
}

void FHorizontalSeparator::setLineWidth(float width)
{
    m_fLineWidth = width;
}

float FHorizontalSeparator::lineWidth() const
{
    return m_fLineWidth;
}
void FHorizontalSeparator::setLinePosition(int pos)
{
    m_iLinePosition = pos;
}

int FHorizontalSeparator::linePosition() const
{
    return m_iLinePosition;
}
void FHorizontalSeparator::setOffsetLeft(int offset)
{
    m_iOffsetLeft = offset;
}

int FHorizontalSeparator::offsetLeft() const
{
    return m_iOffsetLeft;
}

void FHorizontalSeparator::setOffsetRight(int offset)
{
    m_iOffsetRight = offset;
}

int FHorizontalSeparator::offsetRight() const
{
    return m_iOffsetRight;
}
