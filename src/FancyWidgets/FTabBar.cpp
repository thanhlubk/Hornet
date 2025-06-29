#include <FancyWidgets/FTabBar.h>
#include <QToolButton>
#include <FancyWidgets/FUtil.h>
#include <QFrame>
#include <QVBoxLayout>

FTabBar::FTabBar(QWidget *parent)
    : QTabWidget(parent), FThemeableWidget()
{
    setTabPosition(QTabWidget::North);
    setAcceptDrops(true);

    auto* tabBar = new FTabBarPane(this);
    connect(tabBar, &FTabBarPane::tabDropped, this, &FTabBar::recieveTabDropped);
    setTabBar(tabBar);

    m_pCloseButton = new QPushButton(this);
    m_pCloseButton->raise();

    SET_UP_THEME(FTabBar)
}

void FTabBar::applyTheme()
{
    setStyleSheet(QString("QTabWidget::pane { background: %1; border: none; }").arg(getColorTheme(0).name()));

    if (m_pCloseButton)
    {
        m_pCloseButton->setStyleSheet(QString("background: %1; text-align: center; border: none; border-radius: 0px;").arg(getColorTheme(0).name()));
        m_pCloseButton->setFixedSize(getDisplaySize(0).toInt(), tabBar()->height()+2);

        // m_pCloseButton->resize(getDisplaySize(2).toInt(), getDisplaySize(2).toInt()); // Set button size
    }

    updateCloseIcon();
}

QColor FTabBar::getColorTheme(int idx)
{
    SET_UP_COLOR(0, Default, backgroundSection1);
    SET_UP_COLOR(1, Default, textNormal);
    SET_UP_COLOR(2, Default, dominant);

    return QColor(qRgb(0, 0, 0));
}

QVariant FTabBar::getDisplaySize(int idx)
{
    SET_UP_DISPLAY_SIZE(0, Primary, iconSize3);

    return QVariant(0);
}

void FTabBar::recieveTabDropped(FTabBarPane *fromBar, int index)
{
    auto* fromTabWidget = qobject_cast<FTabBar*>(fromBar->parent());
    if (!fromTabWidget || fromTabWidget == this)
        return;

    QWidget* page = fromTabWidget->widget(index);
    QString label = fromTabWidget->tabText(index);
    QIcon icon = fromTabWidget->tabIcon(index);

    fromTabWidget->removeTab(index);
    emit fromTabWidget->tabRemoved(); // Notify container

    page->setParent(this);
    addTab(page, icon, label);
    setCurrentWidget(page);
}

void FTabBar::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat("application/x-tab-index"))
        event->acceptProposedAction();
}

void FTabBar::dropEvent(QDropEvent *event)
{
    auto* sourceTabBar = qobject_cast<FTabBarPane*>(event->source());
    if (!sourceTabBar)
        return;

    QByteArray data = event->mimeData()->data("application/x-tab-index");
    int index = data.toInt();

    emit tabDropped(sourceTabBar, index);
    event->acceptProposedAction();
}

void FTabBar::resizeEvent(QResizeEvent *event)
{
    QTabWidget::resizeEvent(event);

    if (m_pCloseButton)
    {
        QSize size = m_pCloseButton->size();
        m_pCloseButton->move(width() - size.width(), 0);
    }
}

void FTabBar::updateCloseIcon()
{
    QIcon icon = FUtil::changeIconColor(":/FancyWidgets/res/icon/small/close.png", getColorTheme(1));
    m_pCloseButton->setIcon(icon);
    m_pCloseButton->setIconSize(QSize(getDisplaySize(0).toInt(), getDisplaySize(0).toInt())); // Adjust size as needed
}