#pragma once
#include "FancyWidgetsExport.h"
#include "FThemeableWidget.h"
#include <QSplitter>
#include <QBoxLayout>
#include "FSeparator.h"

class FSplitterHandle : public QSplitterHandle
{
    Q_OBJECT
    
public:
    FSplitterHandle(Qt::Orientation orientation, QSplitter *parent)
        : QSplitterHandle(orientation, parent)
    {
        if (orientation == Qt::Horizontal)
            m_pSeparator = new FSeparator(Qt::Vertical, this);
        else
            m_pSeparator = new FSeparator(Qt::Horizontal, this);

        // QBoxLayout *layout = (orientation == Qt::Horizontal)
        //                          ? static_cast<QBoxLayout *>(new QVBoxLayout(this))
        //                          : static_cast<QBoxLayout *>(new QHBoxLayout(this));

        m_pSeparator->setLineWidth(0);
        m_pSeparator->setLinePosition(0);
        m_pSeparator->setOffsetLeft(0);
        m_pSeparator->setOffsetRight(0);
        m_pSeparator->setColorBackground(QColor(0, 0, 0, 0));
        m_pSeparator->setColorLine(QColor(0, 0, 0, 0));

        QBoxLayout *layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(m_pSeparator);
        setLayout(layout);
    }

    FSeparator* separator() { return m_pSeparator; }

private:
    FSeparator* m_pSeparator;
};

class FANCYWIDGETS_EXPORT FSplitter : public QSplitter, public FThemeableWidget
{
    Q_OBJECT
    DECLARE_THEME

public:
    explicit FSplitter(QWidget *parent = nullptr);
    explicit FSplitter(Qt::Orientation orientation, QWidget *parent = nullptr);

    void setHandleSeparatorColor(QColor color);
    void setHandleSeparatorOffset(int offsetStart, int offsetEnd);
    void setHandleWidth(int width);
    void setHandleSpacing(int spacing);

protected:
    QSplitterHandle* createHandle() override;
    void updateHandleSeparatorColor();
    void updateHandleSeparatorOffset();
    void updateHandleSeparatorWidth();

private: 
    QList<QSplitterHandle *> m_listHandle;

    QColor m_colorSeparator;
    std::array<int, 2> m_arrOffset;
    int m_iHandleSpacing;
};