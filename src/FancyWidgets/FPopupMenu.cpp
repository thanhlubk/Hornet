#include <FancyWidgets/FPopupMenu.h>
#include <QStyleOption>
#include <QPainter>

FPopupMenu::FPopupMenu(QWidget* parent)
    : QWidget(parent, Qt::Popup), FThemeableWidget(), m_ePosition(PopupPosition::Bottom) // Important: Qt::Popup makes it behave like a dropdown
{
    setAttribute(Qt::WA_TranslucentBackground, true);
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
    m_pLayout = new QVBoxLayout(this);

    setLayout(m_pLayout);
    SET_UP_THEME(FPopupMenu)
}

void FPopupMenu::applyTheme()
{
    setStyleSheet(QString("background: %1; border-radius: %2px;").arg(getColorTheme(0).name(), QString::number(getDisplaySize(0).toInt())));

    m_pLayout->setSpacing(getDisplaySize(1).toInt());
    m_pLayout->setContentsMargins(getDisplaySize(1).toInt(), getDisplaySize(1).toInt(), getDisplaySize(1).toInt(), getDisplaySize(1).toInt());
}

QColor FPopupMenu::getColorTheme(int idx)
{
    SET_UP_COLOR(0, Default, backgroundSection2);
    return QColor(qRgb(0, 0, 0));
}

QVariant FPopupMenu::getDisplaySize(int idx)
{
    SET_UP_DISPLAY_SIZE(0, Primary, filletSize1);
    SET_UP_DISPLAY_SIZE(1, Primary, offsetSize1);

    return QVariant(0);
}

void FPopupMenu::setPopupPosition(PopupPosition pos)
{
    m_ePosition = pos;
}

void FPopupMenu::addWidget(QWidget* widget)
{
    widget->setParent(this);
    m_pLayout->addWidget(widget);

    applyTheme();
}

void FPopupMenu::hideEvent(QHideEvent* event)
{
    emit popupHidden(); // emit your custom signal
    QWidget::hideEvent(event); // call base class
}

void FPopupMenu::paintEvent(QPaintEvent *event)
{
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void FPopupMenu::appear()
{
    auto pSender = qobject_cast<QWidget*>(sender());
    if (!pSender)
        return;

    QPoint globalPos;

    if (m_ePosition == PopupPosition::Bottom)
        globalPos = pSender->mapToGlobal(pSender->rect().bottomLeft() + QPoint(0, getDisplaySize(0).toInt()));
    else if (m_ePosition == PopupPosition::Right)
        globalPos = pSender->mapToGlobal(pSender->rect().topRight());

    move(globalPos);
    setFocus();
    show();
    raise();
}