#pragma once
#include "FancyWidgetsExport.h"
#include <QWidget>

class FANCYWIDGETS_EXPORT FHorizontalSeparator : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QColor colorLine READ colorLine WRITE setColorLine)
    Q_PROPERTY(QColor colorBackground READ colorBackground WRITE setColorBackground)
    Q_PROPERTY(float lineWidth READ lineWidth WRITE setLineWidth)
    Q_PROPERTY(int linePosition READ linePosition WRITE setLinePosition)
    Q_PROPERTY(int offsetLeft READ offsetLeft WRITE setOffsetLeft)
    Q_PROPERTY(int offsetRight READ offsetRight WRITE setOffsetRight)

public:
    explicit FHorizontalSeparator(QWidget* parent = nullptr);

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

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QColor m_colorLine;
    QColor m_colorBackground;
    float m_fLineWidth;
    int m_iLinePosition;
    int m_iOffsetLeft;
    int m_iOffsetRight;
};