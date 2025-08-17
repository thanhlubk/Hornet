#pragma once
#include "FancyWidgetsExport.h"
#include <QWidget>
#include <array>

class FANCYWIDGETS_EXPORT FSeparator : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QColor colorLine READ colorLine WRITE setColorLine)
    Q_PROPERTY(QColor colorBackground READ colorBackground WRITE setColorBackground)
    Q_PROPERTY(float lineWidth READ lineWidth WRITE setLineWidth)
    Q_PROPERTY(int linePosition READ linePosition WRITE setLinePosition)
    Q_PROPERTY(int offsetLeft READ offsetLeft WRITE setOffsetLeft)
    Q_PROPERTY(int offsetRight READ offsetRight WRITE setOffsetRight)

public:
    explicit FSeparator(Qt::Orientation orientation, QWidget *parent = nullptr);

    void setColorLine(const QColor& color);
    QColor colorLine() const;
    void setColorBackground(const QColor& color);
    QColor colorBackground() const;
    void setLineWidth(float width);
    float lineWidth() const;
    void setLinePosition(int pos);
    int linePosition() const;
    void setOffsetLeft(int offset);
    int offsetLeft() const;
    void setOffsetRight(int offset);
    int offsetRight() const;
    void setOffsetTop(int offset);
    int offsetTop() const;
    void setOffsetBottom(int offset);
    int offsetBottom() const;
    void setOffset(int offsetStart, int offsetEnd);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    Qt::Orientation m_eOrientation;
    QColor m_colorLine;
    QColor m_colorBackground;
    float m_fLineWidth;
    int m_iLinePosition;
    std::array<int, 2> m_arrOffset; // [left, right] or [top, bottom]
};