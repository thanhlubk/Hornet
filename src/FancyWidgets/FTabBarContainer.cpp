#include <FancyWidgets/FTabBarContainer.h>
#include <QTimer>
#include <QLabel>
#include <QStyleOption>
#include <QPainter>
#include <FancyWidgets/FUtil.h>

FTabBarContainer::FTabBarContainer(QWidget *parent)
    : QWidget(parent), FThemeableWidget()
{
    m_pSplitter = new QSplitter(Qt::Horizontal, this);
    m_pSplitter->setChildrenCollapsible(false); // Prevent full collapse
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_pSplitter);
    // addTabWidget(); // Start with 1
    // m_pSplitter->setStyleSheet("background: green; border: none;");

    SET_UP_THEME(FTabBarContainer)
}

void FTabBarContainer::applyTheme()
{
    // setStyleSheet(QString("QTabWidget::pane { background: %1; border:none; }").arg(getColorTheme(0).name()));
    const QObjectList &listChild = m_pSplitter->children();
    for (QObject *pChild : listChild)
    {
        auto pWidget = qobject_cast<QWidget *>(pChild);
        if (pWidget)
            updateStyleContainer(pWidget);
    }

    m_pSplitter->setHandleWidth(getDisplaySize(2).toInt()); // Optional: custom handle width
}

QColor FTabBarContainer::getColorTheme(int idx)
{
    SET_UP_COLOR(0, Default, backgroundSection1);
    SET_UP_COLOR(1, Default, dominant);

    return QColor(qRgb(0, 0, 0));
}

QVariant FTabBarContainer::getDisplaySize(int idx)
{
    SET_UP_DISPLAY_SIZE(0, Primary, offsetSize2);
    SET_UP_DISPLAY_SIZE(1, Primary, filletSize2);
    SET_UP_DISPLAY_SIZE(2, Primary, offsetSize1);

    return QVariant(0);
}

#if 0
void FTabBarContainer::addTabWidget()
{
    QWidget *pContainer = new QWidget(this);
    auto *layout = new QHBoxLayout(pContainer);
    layout->setContentsMargins(10, 10, 10, 10);

    FTabBar *pTabBar = new FTabBar(this);
    layout->addWidget(pTabBar);
    pContainer->setLayout(layout);

    // widget->setStyleSheet("background: blue; border:none; border-radius: 10px;");

    m_pSplitter->addWidget(pContainer);

    auto qLabel1 = new QLabel("Tab 1 in Widget 1", pTabBar);
    qLabel1->setStyleSheet("background: yellow; border: none;");
    auto qLabel2 = new QLabel("Tab 2 in Widget 2", pTabBar);
    qLabel2->setStyleSheet("background: yellow; border: none;");
    pTabBar->addTab(qLabel1, "Tab 1");
    pTabBar->addTab(qLabel2, "Tab 2");

    connect(pTabBar, &QTabWidget::tabCloseRequested, this, &FTabBarContainer::onTabClosed);
    connect(pTabBar, &FTabBar::tabRemoved, this, [=]()
            { QTimer::singleShot(0, this, &FTabBarContainer::cleanupEmptyTabWidgets); });
}
#endif

FTabBar* FTabBarContainer::addTabBar(int index)
{
    QWidget *pContainer = new QWidget(this);
    auto *layout = new QHBoxLayout(pContainer);
    // pContainer->setStyleSheet("background: red; border-radius: 10px; border: none;");

    FTabBar *pTabBar = new FTabBar(pContainer);
    layout->addWidget(pTabBar);
    pContainer->setLayout(layout);
    if (index == -1)
        m_pSplitter->addWidget(pContainer);
    else
        m_pSplitter->insertWidget(index, pContainer);
    connect(pTabBar, &QTabWidget::tabCloseRequested, this, &FTabBarContainer::closeTab);
    connect(pTabBar, &FTabBar::tabDropped, this, &FTabBarContainer::splitTab);
    connect(pTabBar, &FTabBar::tabRemoved, this, [=]()
            { QTimer::singleShot(0, this, &FTabBarContainer::cleanupEmptyTabWidgets); });

    updateStyleContainer(pContainer);
    return pTabBar;
}

void FTabBarContainer::addPane(unsigned short index, const QString &title, QWidget *widget)
{
    if (m_pSplitter->count() < 1)
        addTabBar();

    if (index >= m_pSplitter->count())
        index = 0;

    QWidget *pContainer = m_pSplitter->widget(index);
    if (!pContainer)
        return;

    FTabBar *pTabBar = nullptr;
    const QObjectList &listChild = pContainer->children();
    for (QObject *pChild : listChild)
    {
        pTabBar = qobject_cast<FTabBar *>(pChild);
        if (pTabBar)
            break;
    }

    if (!pTabBar)
        return;

    widget->setParent(pTabBar);
    pTabBar->addTab(widget, title);
    pTabBar->setTabIcon(pTabBar->count()-1, FUtil::createNumberIcon(1, getColorTheme(1), QColor("white")));
    pTabBar->setIconSize(QSize(15, 15));
}

void FTabBarContainer::addPane(const QString &title, QWidget *widget)
{
    addPane(0, title, widget);
}

void FTabBarContainer::cleanupEmptyTabWidgets()
{
    for (int i = m_pSplitter->count() - 1; i >= 0; --i)
    {
        bool bDelete = false;
        QWidget *pContainer = m_pSplitter->widget(i);
        const QObjectList &listChild = pContainer->children();
        for (QObject *pChild : listChild)
        {
            FTabBar *pTabBar = qobject_cast<FTabBar *>(pChild);
            if (pTabBar && pTabBar->count() == 0)
            {
                bDelete = true;
                break;
            }
        }

        if (bDelete)
        {
            pContainer->hide();
            pContainer->deleteLater();
        }
    }
}

void FTabBarContainer::closeTab(int index)
{
    Q_UNUSED(index);
    cleanupEmptyTabWidgets();
}

void FTabBarContainer::splitTab(FTabBarPane *fromBar, int index)
{
    FTabBar* pTabBarRecieve = qobject_cast<FTabBar*>(sender());
    if (!pTabBarRecieve)
        return;

    auto *pTabBarSend = qobject_cast<FTabBar *>(fromBar->parent());
    if (!pTabBarSend)
        return;

    auto pContainer = qobject_cast<QWidget *>(pTabBarRecieve->parent());
    if (!pContainer)
        return;

    QWidget *page = pTabBarSend->widget(index);
    QString label = pTabBarSend->tabText(index);
    QIcon icon = pTabBarSend->tabIcon(index);

    pTabBarSend->removeTab(index);
    emit pTabBarSend->tabRemoved();

    int idx = m_pSplitter->indexOf(pContainer);
    if (idx != -1)
        idx += 1;

    FTabBar *pTabBarSplit = addTabBar(idx);

    page->setParent(pTabBarSplit);
    pTabBarSplit->addTab(page, icon, label);
    pTabBarSplit->setCurrentWidget(page);
}

void FTabBarContainer::paintEvent(QPaintEvent *event)
{
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void FTabBarContainer::updateStyleContainer(QWidget *widget)
{
    if (!widget)
        return;

    auto pLayout = widget->layout();
    pLayout->setContentsMargins(getDisplaySize(0).toInt(), 0, getDisplaySize(0).toInt(), getDisplaySize(0).toInt());

    widget->setStyleSheet(QString("background: %1; border: none; border-radius: %2px;").arg(getColorTheme(0).name(), QString::number(getDisplaySize(1).toInt())));
}