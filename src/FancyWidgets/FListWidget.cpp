#include <FancyWidgets/FListWidget.h>

FListWidgetResizeWatcher::FListWidgetResizeWatcher(QListWidgetItem *item, QWidget *watched, QListWidget *list, QObject *parent)
    : QObject(parent), m_pListItem(item), m_pList(list)
{
    watched->installEventFilter(this);
}

bool FListWidgetResizeWatcher::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::Resize)
    {
        qDebug() << "Resized event";
        QWidget *widget = qobject_cast<QWidget *>(obj);
        if (widget && m_pListItem && m_pList)
        {
            // if (m_pList->viewMode() == QListView::IconMode)
            // {
            //     // In IconMode, we might want to adjust the size hint differently
            //     m_pListItem->setSizeHint(widget->size());
            // }
            // else
            // {
            //     // In other modes, we can set the size hint directly
            //     auto viewPortWidth = m_pList->viewport()->width() - 2 * m_pList->spacing();
            //     QSize newSize = widget->size();
            //     newSize.setWidth(viewPortWidth); // Ensure it fits the view width
            //     m_pListItem->setSizeHint(newSize);
            //     qDebug() << "Resized item to:" << newSize;
            // }

            // m_pListItem->setSizeHint(widget->size());

            m_pListItem->setSizeHint(QSize(m_pListItem->sizeHint().width(), widget->size().height()));
            m_pList->update();
        }
    }
    return QObject::eventFilter(obj, event);
}

FListWidget::FListWidget(QWidget *parent)
    : QListWidget(parent), m_bEnableSpacingBorder(false), m_marginsOverride(0, 0, 0, 0)
{
    setResizeMode(QListView::Adjust);
    setWordWrap(true);
    setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    setFocusPolicy(Qt::NoFocus);
    setFrameStyle(QFrame::NoFrame);
}

void FListWidget::addWidget(QWidget *widget)
{
    if (!widget)
        return;

    QListWidgetItem *item = new QListWidgetItem(this);
    item->setSizeHint(widget->size());
    this->addItem(item);
    this->setItemWidget(item, widget);

    // Install event filter to monitor resize events
    widget->installEventFilter(this);
    // Store item-widget mapping for efficient lookup
    m_mapItemWidget.insert(widget, item);

    // Attach the resize watcher
    // new FListWidgetResizeWatcher(item, widget, this, widget); // Memory safe: QObject parented to widget
}

void FListWidget::enableSpacingBorder(bool enable)
{
    m_bEnableSpacingBorder = enable;
    updateViewPort();
}

bool FListWidget::isSpacingBorderEnabled() const
{
    return m_bEnableSpacingBorder;
}

// Override setSpacing to call custom function
void FListWidget::setSpacing(int spacing)
{
    QListWidget::setSpacing(spacing); // Call base class implementation
    updateViewPort();  // Call custom function
}

void FListWidget::setContentsMargins(int left, int top, int right, int bottom)
{
    setContentsMargins(QMargins(left, top, right, bottom));
}

void FListWidget::setContentsMargins(const QMargins &margins)
{
    m_marginsOverride = margins;
}

QMargins FListWidget::contentsMargins() const
{
    return m_marginsOverride;
}

QMargins FListWidget::adjustViewportMargins()
{
    QMargins margins = QMargins(0, 0, 0, 0);
    if (!m_bEnableSpacingBorder)
        margins = QMargins(-spacing(), 0, -spacing() - 1, 0);

    margins = margins + m_marginsOverride; // Combine with any overridden margins
    return margins;
}

void FListWidget::updateViewPort()
{
    auto margins = adjustViewportMargins();
    setViewportMargins(margins);

    // if (m_bEnableSpacingBorder)
    //     setViewportMargins(0, 0, 0, 0);
    // else
    //     setViewportMargins(-spacing(), 0, -spacing() - 1, 0);
}

bool FListWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::Resize)
    {
        QWidget* widget = qobject_cast<QWidget *>(obj);
        if (widget && m_mapItemWidget.contains(widget))
        {
            QListWidgetItem *item = m_mapItemWidget.value(widget);
            item->setSizeHint(QSize(item->sizeHint().width(), widget->height()));
            updateGeometries(); // Ensure immediate layout update
        }
    }
    return QListWidget::eventFilter(obj, event); // Pass event to parent
}