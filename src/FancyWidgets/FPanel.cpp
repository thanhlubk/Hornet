#include <FancyWidgets/FPanel.h>
#include <QVBoxLayout>
#include <QListWidget>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QEvent>
#include <QPainter>
#include <QPainterPath>

FPanel::FPanel(QWidget* parent)
    : QWidget(parent), FThemeableWidget(), m_bRun(false)
{
    m_pLayout = new QVBoxLayout(this);
    m_pLayout->setSpacing(0);
    m_pLayout->setContentsMargins(0, 0, 0, 0);

    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    m_pButtonHeader = new FArrowButton("View", "", ":/FancyWidgets/res/icon/small/forward.png", false, this);
    m_pButtonHeader->enableBold(false);
    m_pButtonHeader->enableMoveAnimation(true);

    // Content: QListWidget
    m_pListContent = new FNoScrollListWidget(this);
    m_pListContent->setViewMode(QListView::ListMode);           // List mode
    m_pListContent->setFlow(QListView::LeftToRight);            // Flow left to right
    m_pListContent->setWrapping(true);                          // Wrap into rows
    m_pListContent->setResizeMode(QListView::Adjust);           // Adjust on resize
    m_pListContent->setWordWrap(true);
    m_pListContent->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    m_pListContent->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_pListContent->setMinimumHeight(0);       // Allows collapsing to zero
    m_pListContent->setMaximumHeight(0);     // Initial max width
    m_pListContent->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_pListContent->setFocusPolicy(Qt::NoFocus);
    m_pListContent->setFrameStyle(QFrame::NoFrame);

    m_pLayout->addWidget(m_pButtonHeader);
    m_pLayout->addWidget(m_pListContent);
    m_pLayout->addStretch();

    observeWidget(m_pButtonHeader);
    observeWidget(m_pListContent);

    updateContainerHeight();

    connect(m_pButtonHeader, &FArrowButton::arrowToggled, this, &FPanel::onArrowToggled);

    SET_UP_THEME(FPanel)
}

void FPanel::applyTheme() 
{
    m_pListContent->setSpacing(getDisplaySize(0).toInt());
    m_pListContent->enableSpacingBorder(false);
    m_pListContent->setStyleSheet(QString("QListWidget {background-color: %1; } QListWidget::item { outline: none; border: none; } QListWidget::item:selected { background: %1; outline: none; } QListWidget::item:focus { outline: none; }").arg(getColorTheme(1).name(QColor::HexArgb)));

    setStyleSheet(QString("background-color: %1;").arg(getColorTheme(0).name()));
}

QColor FPanel::getColorTheme(int idx)
{
    SET_UP_COLOR(0, Default, backgroundSection1);
    SET_UP_COLOR(1, Default, transparent);

    return QColor(qRgb(0, 0, 0));
}

QVariant FPanel::getDisplaySize(int idx)
{
    SET_UP_DISPLAY_SIZE(0, Primary, offsetSize1);

    return QVariant(0);
}

void FPanel::paintEvent(QPaintEvent* event)
{
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void FPanel::observeWidget(QWidget* child)
{
    if (!child || m_listObservedChildren.contains(child))
        return;

    m_listObservedChildren.append(child);
    child->installEventFilter(this);
}


bool FPanel::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::Resize)
    {
        updateContainerHeight();
    }
    return QWidget::eventFilter(watched, event);
}


void FPanel::updateContainerHeight() {
    int totalHeight = 0;
    for (int i = 0; i < m_pLayout->count(); ++i)
    {
        QWidget* widget = m_pLayout->itemAt(i)->widget();
        if (widget)
        {
            totalHeight += widget->height();
        }
    }

    resize(width(), totalHeight);         // only update width
    setFixedHeight(totalHeight);            // lock it so parent layout respects it
}

void FPanel::onArrowToggled(bool expanded) 
{
    m_bRun = true;
    int startHeight = m_iExpandHeight;
    int endHeight = 0;
    int animationTime = 200;
    if (expanded)
    {
        endHeight = startHeight;
        startHeight = 0;
    }

    QPropertyAnimation* anim = new QPropertyAnimation(m_pListContent, "maximumHeight");
    anim->setDuration(animationTime);
    anim->setStartValue(startHeight);
    anim->setEndValue(endHeight);
    anim->setEasingCurve(QEasingCurve::OutQuad);

    QPropertyAnimation* anim2 = new QPropertyAnimation(m_pListContent, "minimumHeight");
    anim2->setDuration(animationTime);
    anim2->setStartValue(startHeight);
    anim2->setEndValue(endHeight);
    // Create the parallel animation group
    QParallelAnimationGroup* group = new QParallelAnimationGroup;

    // Add animations to the group
    group->addAnimation(anim);
    group->addAnimation(anim2);

    connect(anim, &QVariantAnimation::valueChanged, this, [=]() {
        this->updateContainerHeight();  // manually recalculate total width
    });
    group->start(QAbstractAnimation::DeleteWhenStopped);
    connect(anim, &QAbstractAnimation::finished, this, & FPanel::updateContainerHeight);
    connect(anim, &QAbstractAnimation::finished, this, [=]() {
        this->setRun(false);  // manually recalculate total width
    });
}

void FPanel::updateExpandHeight()
{
    // auto listWidgetHeight = 2*10;
    auto listWidgetHeight = 0 + m_pListContent->spacing();

    int topPos = 0;
    int bottomPos = 0;
    int backupBottomPos = 0;

    if (m_pListContent->count() > 0)
    {
        auto firstItem = m_pListContent->item(0);
        QModelIndex firstItemIndex = m_pListContent->indexFromItem(firstItem);

        // Get the rectangle occupied by the item in the view's coordinates
        QRect itemRect = m_pListContent->visualRect(firstItemIndex);

        topPos = itemRect.topLeft().y();
        backupBottomPos = itemRect.bottomLeft().y();
    }

    if (m_pListContent->count() > 0)
    {
        auto lastItem = m_pListContent->item(m_pListContent->count() - 1);

        QModelIndex lastItemIndex = m_pListContent->indexFromItem(lastItem);

        // Get the rectangle occupied by the item in the view's coordinates
        QRect itemRect = m_pListContent->visualRect(lastItemIndex);

        bottomPos = itemRect.bottomLeft().y();
    }
    
    if (backupBottomPos > bottomPos)
        bottomPos = backupBottomPos;
    
    listWidgetHeight += bottomPos - topPos;

    if (!m_bRun && m_pButtonHeader->isExpand())
    {
        m_pListContent->setMinimumHeight(listWidgetHeight);
        m_pListContent->setMaximumHeight(listWidgetHeight);
    }

    m_iExpandHeight = listWidgetHeight;
}

void FPanel::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    updateExpandHeight();
    emit heightChanged();
}

void FPanel::setRun(bool run)
{
    m_bRun = run;
}

void FPanel::addWidget(QWidget* widget)
{
    QListWidgetItem* item = new QListWidgetItem(m_pListContent);
    item->setSizeHint(widget->size());
    m_pListContent->addItem(item);
    m_pListContent->setItemWidget(item, widget);
}