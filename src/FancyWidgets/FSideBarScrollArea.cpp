#include <FancyWidgets/FSideBarScrollArea.h>
#include <FancyWidgets/FSideBarTab.h>
#include <FancyWidgets/FUtil.h>
#include <QStyleOption>
#include <QScrollBar>
#include <QTimer>
#include <QIcon>

#define SCROLL_STEP 5

FSideBarScrollArea::FSideBarScrollArea(QWidget* parent) 
    : QWidget(parent), FThemeableWidget(), m_pCurrentWidget(nullptr)
{
    m_pMainLayout = new QVBoxLayout(this);
    m_pMainLayout->setSpacing(0);
    m_pMainLayout->setContentsMargins(0, 0, 0, 0);

    auto pMainWindow = qobject_cast<QWidget*>(this->parent()->parent());
    if (pMainWindow)
        m_pScrollUpButton = new QPushButton("", pMainWindow);
    else
        m_pScrollUpButton = new QPushButton("", this);

    m_pScrollDownButton = new QPushButton("", this);

    m_pScrollArea = new QScrollArea(this);
    m_pScrollArea->setWidgetResizable(true);
    m_pScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_pScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_pScrollArea->setFrameStyle(QFrame::NoFrame);

    m_pScrollContent = new QWidget(this);

    m_pContentLayout = new QVBoxLayout(m_pScrollContent);
    m_pContentLayout->setAlignment(Qt::AlignTop);
    m_pContentLayout->setSpacing(0);
    m_pContentLayout->setContentsMargins(0, 0, 0, 0);
    m_pScrollContent->setLayout(m_pContentLayout);

    m_pScrollArea->setWidget(m_pScrollContent);

    m_pMainLayout->addWidget(m_pScrollArea);
    m_pMainLayout->addItem(new QSpacerItem(0, 80, QSizePolicy::Fixed, QSizePolicy::Minimum));

    m_pScrollUpButton->raise();
    m_pScrollDownButton->raise();

    m_vePinnedButton.reserve(2);

    connect(m_pScrollUpButton, &QPushButton::clicked, this, &FSideBarScrollArea::scrollUp);
    connect(m_pScrollDownButton, &QPushButton::clicked, this, &FSideBarScrollArea::scrollDown);

    connect(m_pScrollArea->verticalScrollBar(), &QScrollBar::valueChanged,
        this, [this]() { this->updateScrollButtonVisibility(); });

    SET_UP_THEME(FSideBarScrollArea)
}

void FSideBarScrollArea::applyTheme() {
    updateFrameColor();

    m_pScrollUpButton->setStyleSheet(QString("background-color: %2; border: none; border-radius: %1px; text-align: center;").arg(QString::number(getDisplaySize(3).toInt()), getColorTheme(1).name()));
    m_pScrollDownButton->setStyleSheet(QString("background-color: %2; border: none; border-radius: %1px; text-align: center;").arg(QString::number(getDisplaySize(3).toInt()), getColorTheme(1).name()));
    setStyleSheet("background-color: transparent; border: none; ");

    updateScrollIcon();

    m_pScrollUpButton->resize(26, getDisplaySize(0).toInt()); // Set button size
    m_pScrollDownButton->resize(26, getDisplaySize(0).toInt()); // Set button size

}

QColor FSideBarScrollArea::getColorTheme(int idx)
{
    SET_UP_COLOR(0, Default, backgroundSection2);
    SET_UP_COLOR(1, Default, backgroundSection1);
    SET_UP_COLOR(2, Default, textNormal);
    return QColor(qRgb(0, 0, 0));
}

QVariant FSideBarScrollArea::getDisplaySize(int idx)
{
    SET_UP_DISPLAY_SIZE(0, Primary, iconSize3);
    SET_UP_DISPLAY_SIZE(1, Primary, offsetSize1);
    SET_UP_DISPLAY_SIZE(2, Primary, offsetSize2);
    SET_UP_DISPLAY_SIZE(3, Primary, filletSize1);

    return QVariant(0);
}

void FSideBarScrollArea::paintEvent(QPaintEvent* event)
{
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void FSideBarScrollArea::updateCurrentTabPosition()
{
    if (!m_pCurrentWidget)
        return;

    QPoint pos = m_pScrollArea->viewport()->mapFromGlobal(m_pCurrentWidget->mapToGlobal(QPoint(0, 0)));
    emit scrollingPositionChanged(pos.y(), pos.y() + m_pCurrentWidget->height(), false);
}

void FSideBarScrollArea::updateTabView(QWidget *currentTab)
{
    auto pSender = qobject_cast<FSideBarTab*>(currentTab);
    m_pCurrentWidget = pSender;

    for (int i = 0; i < m_pContentLayout->count(); ++i) 
    {
        auto pButton = qobject_cast<FSideBarTab*>(m_pContentLayout->itemAt(i)->widget());
        if (pButton && pButton != pSender) {
            pButton->setChecked(false);
        }
    }
    for (auto button : m_vePinnedButton)
    {
        auto pButton = qobject_cast<FSideBarTab*>(button);
        if (pButton && pButton != pSender) {
            pButton->setChecked(false);
        }
    }
    
    for (auto frame : m_veFrame) 
    {
        frame->enablePaint(false);
    }
    for (auto frame : m_vePinnedFrame) 
    {
        frame->enablePaint(false);
    }

    if (pSender)
    {
        auto bPinned = pSender->property("pinned").toBool();
        auto frameIdx = pSender->property("frameIdx").toInt();
        if (!bPinned)
        {
            if (frameIdx < m_veFrame.size())
            {
                m_veFrame[frameIdx]->setAbove(false);
                m_veFrame[frameIdx]->enablePaint(true);
            }

            if (frameIdx != 0)
            {
                m_veFrame[frameIdx - 1]->setAbove(true);
                m_veFrame[frameIdx - 1]->enablePaint(true);
            }
        }
        else
        {
            if (frameIdx < m_vePinnedFrame.size())
            {
                m_vePinnedFrame[frameIdx]->setAbove(true);
                m_vePinnedFrame[frameIdx]->enablePaint(true);
            }

            if (frameIdx != m_vePinnedFrame.size() - 1)
            {
                m_vePinnedFrame[frameIdx + 1]->setAbove(false);
                m_vePinnedFrame[frameIdx + 1]->enablePaint(true);
            }
        }
        
        if (!bPinned)
            updateCurrentTabPosition();
        else
        {
            emit scrollingPositionChanged(pSender->y(), pSender->y() + pSender->height(), true);
        }
    }
    update();
}

void FSideBarScrollArea::updateScrollButtonVisibility()
{
    QScrollBar* vBar = m_pScrollArea->verticalScrollBar();
    int max = vBar->maximum();
    int min = vBar->minimum();
    int val = vBar->value();

    m_pScrollUpButton->setVisible(val > min);
    m_pScrollDownButton->setVisible(val < max);

    if (m_pCurrentWidget)
        updateCurrentTabPosition();
    
}

void FSideBarScrollArea::addItem(QWidget* item)
{
    item->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_pContentLayout->addWidget(item);
    FSideBarTabFilletFrame* pFrame = new FSideBarTabFilletFrame(this);
    pFrame->setFixedHeight(getDisplaySize(1).toInt());
    m_pContentLayout->addWidget(pFrame);
    QTimer::singleShot(0, this, &FSideBarScrollArea::updateScrollButtonVisibility);

    m_veFrame.append(pFrame);

    auto pButton = qobject_cast<FSideBarTab*>(item);
    if (pButton)
    {
        connect(pButton, &FSideBarTab::clicked, this, &FSideBarScrollArea::clickTab);
        pButton->setProperty("frameIdx", static_cast<int>(m_veFrame.size() - 1));
        pButton->setProperty("pinned", false);
    }

    updateFrameColor();
}

void FSideBarScrollArea::addPinItem(QWidget* item)
{
    item->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_vePinnedButton.append(item);

    FSideBarTabFilletFrame* pFrame = new FSideBarTabFilletFrame(this);
    m_pMainLayout->addWidget(pFrame);
    m_pMainLayout->addWidget(item);
    item->lower();
    QTimer::singleShot(0, this, &FSideBarScrollArea::updateScrollButtonVisibility);

    m_vePinnedFrame.append(pFrame);

    auto pButton = qobject_cast<FSideBarTab*>(item);
    if (pButton)
    {
        connect(pButton, &FSideBarTab::clicked, this, &FSideBarScrollArea::clickTab);
        pButton->setProperty("frameIdx", static_cast<int>(m_vePinnedFrame.size() - 1));
        pButton->setProperty("pinned", true);
    }

    updateFrameColor();
}

void FSideBarScrollArea::scrollUp()
{
    QScrollBar* pScrollBar = m_pScrollArea->verticalScrollBar();
    pScrollBar->setValue(pScrollBar->value() - SCROLL_STEP);
}

void FSideBarScrollArea::scrollDown()
{
    QScrollBar* pScrollBar = m_pScrollArea->verticalScrollBar();
    pScrollBar->setValue(pScrollBar->value() + SCROLL_STEP);
}

void FSideBarScrollArea::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);

    if (m_pScrollUpButton && m_pScrollDownButton)
    {
        m_pScrollDownButton->move((m_pScrollArea->width() - m_pScrollDownButton->width() - getDisplaySize(2).toInt())/2,  m_pScrollArea->height() + getDisplaySize(2).toInt() - m_pScrollDownButton->height());

        QPoint position(0, 0);
        auto pMainWindow = qobject_cast<QWidget*>(this->parent()->parent());
        if (pMainWindow)
            position = pMainWindow->mapFromGlobal(this->mapToGlobal(QPoint(0, 0)));
        
        m_pScrollUpButton->move(position.x() + (m_pScrollArea->width() - m_pScrollUpButton->width() - getDisplaySize(2).toInt())/2, position.y() - getDisplaySize(2).toInt());

        m_pScrollUpButton->raise();
        m_pScrollDownButton->raise();
    }
}

void FSideBarScrollArea::uncheckAllTab()
{
    auto pTab = qobject_cast<FSideBarTab*>(m_pCurrentWidget);
    if (pTab)
        pTab->setChecked(false);
    
    updateTabView(nullptr);
}

void FSideBarScrollArea::clickTab()
{
    auto pSender = qobject_cast<FSideBarTab*>(sender());
    if (!pSender)
        return;

    updateTabView(pSender);
}

void FSideBarScrollArea::updateFrameColor()
{
    for (auto pFrame : m_veFrame)
    {
        pFrame->setBrushColor(getColorTheme(1));
        pFrame->setFixedHeight(getDisplaySize(1).toInt());
    }

    for (auto pFrame : m_vePinnedFrame)
    {
        pFrame->setBrushColor(getColorTheme(1));
        pFrame->setFixedHeight(getDisplaySize(1).toInt());
    }
}

void FSideBarScrollArea::updateScrollIcon()
{
    QIcon icon = FUtil::changeIconColor(":/FancyWidgets/res/icon/small/scroll-up.png", getColorTheme(2));
    m_pScrollUpButton->setIcon(icon);
    m_pScrollUpButton->setIconSize(QSize(getDisplaySize(0).toInt(), getDisplaySize(0).toInt())); // Adjust size as needed

    auto size = FUtil::getIconLargestSize(icon);
    auto pixmap = icon.pixmap(size);
    QTransform transform;
    transform.rotate(180); // Rotate by 180 degrees
    QPixmap rotatedPixmap = pixmap.transformed(transform); // Apply transformation

    QIcon rotatedIcon(rotatedPixmap);
    m_pScrollDownButton->setIcon(rotatedIcon);
    m_pScrollDownButton->setIconSize(QSize(getDisplaySize(0).toInt(), getDisplaySize(0).toInt()));
}