#include <FancyWidgets/FPanelSplitButton.h>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QDebug>
#include <QMouseEvent>
#include <QPainter>
#include <QStyleOption>
#include <FancyWidgets/FUtil.h>
#include <FancyWidgets/FExpandableButton.h>
#include <QPushButton>
#include <QWidget>

FPanelSplitButton::FPanelSplitButton(QWidget *parent)
    : QWidget(parent), FThemeableWidget(), m_bKeep(false), m_strIcon("")
{
    setMouseTracking(true);
    setCursor(Qt::PointingHandCursor);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    m_pLabelIcon = new QLabel(this);
    m_pLabelIcon->setAlignment(Qt::AlignCenter);
    m_pLabelText = new QLabel(this);
    m_pLabelText->setAlignment(Qt::AlignCenter);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(2, 2, 2, 2);
    layout->setSpacing(0);
    layout->addWidget(m_pLabelIcon);
    layout->addWidget(m_pLabelText);

    m_pMenu = new FPopupMenu(this);

    connect(this, &FPanelSplitButton::textClicked, m_pMenu, &FPopupMenu::appear);
    connect(m_pMenu, &FPopupMenu::popupHidden, [this]() { m_eHoverArea = None; updateStyle(); });

    SET_UP_THEME(FPanelSplitButton)
}

void FPanelSplitButton::applyTheme()
{
    updateStyle();
}

QColor FPanelSplitButton::getColorTheme(int idx)
{
    SET_UP_COLOR(0, Default, backgroundSection2);
    SET_UP_COLOR(1, Default, widgetCheck);
    SET_UP_COLOR(2, Default, textHeader2);
    SET_UP_COLOR(3, Default, textNormal);

    return QColor(qRgb(0, 0, 0));
}

QVariant FPanelSplitButton::getDisplaySize(int idx)
{
    SET_UP_DISPLAY_SIZE(0, Primary, filletSize1);
    SET_UP_DISPLAY_SIZE(1, Primary, fontNormalSize);
    SET_UP_DISPLAY_SIZE(2, Primary, iconSize2);

    return QVariant(0);
}

void FPanelSplitButton::setIcon(const QString &icon, bool keep)
{
    m_strIcon = icon;
    m_bKeep = keep;
    updateIcon();
}

void FPanelSplitButton::setText(const QString &text)
{
    m_pLabelText->setText(text);
}

void FPanelSplitButton::addWidget(QWidget *widget)
{
    if (!m_pMenu)
        return;

    m_pMenu->addWidget(widget);
}

void FPanelSplitButton::mousePressEvent(QMouseEvent *event)
{
    QPoint pos = event->pos();

    if (m_pLabelIcon->geometry().contains(pos))
    {
        emit iconClicked();
    }
    else if (m_pLabelText->geometry().contains(pos))
    {
        emit textClicked();
    }

    QWidget::mousePressEvent(event);
}

void FPanelSplitButton::enterEvent(QEnterEvent *event)
{
    updateStyle();
    QWidget::enterEvent(event);
}

void FPanelSplitButton::leaveEvent(QEvent *event)
{
    m_eHoverArea = None;
    updateStyle();
    QWidget::leaveEvent(event);
}

void FPanelSplitButton::mouseMoveEvent(QMouseEvent *event)
{
    QPoint pos = event->pos();
    if (m_pLabelIcon->geometry().contains(pos))
        m_eHoverArea = Icon;
    else if (m_pLabelText->geometry().contains(pos))
        m_eHoverArea = Text;
    else
        m_eHoverArea = None;

    updateStyle();
    QWidget::mouseMoveEvent(event);
}

void FPanelSplitButton::paintEvent(QPaintEvent *event)
{
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void FPanelSplitButton::updateStyle()
{
    setStyleSheet(QString("QWidget { background: %1; border-radius: %2px; } QLabel { border: none; color: %3}").arg(m_eHoverArea == 0 && m_pMenu->isHidden() ? getColorTheme(0).name() : getColorTheme(1).name(), QString::number(getDisplaySize(0).toInt()), getColorTheme(2).name()));

    if (m_eHoverArea == Icon)
    {
        m_pLabelIcon->setStyleSheet(QString("background-color: %1; border-top-left-radius: %2px; border-top-right-radius: %2px; border-bottom-left-radius: 0px; border-bottom-right-radius: 0px;").arg(getColorTheme(1).name(), QString::number(getDisplaySize(0).toInt())));
        m_pLabelText->setStyleSheet(QString("background-color: %1; border-top-left-radius: 0px; border-top-right-radius: 0px; border-bottom-left-radius: %2px; border-bottom-right-radius: %2px;").arg(getColorTheme(0).name(), QString::number(getDisplaySize(0).toInt())));
    }
    else if (m_eHoverArea == Text || !m_pMenu->isHidden())
    {
        m_pLabelText->setStyleSheet(QString("background-color: %1; border-top-left-radius: 0px; border-top-right-radius: 0px; border-bottom-left-radius: %2px; border-bottom-right-radius: %2px;").arg(getColorTheme(1).name(), QString::number(getDisplaySize(0).toInt())));
        m_pLabelIcon->setStyleSheet(QString("background-color: %1; border-top-left-radius: %2px; border-top-right-radius: %2px; border-bottom-left-radius: 0px; border-bottom-right-radius: 0px;").arg(getColorTheme(0).name(), QString::number(getDisplaySize(0).toInt())));
    }
    else
    {
        m_pLabelIcon->setStyleSheet(QString("background-color: %1;").arg(getColorTheme(0).name()));
        m_pLabelText->setStyleSheet(QString("background-color: %1;").arg(getColorTheme(0).name()));
    }

    FUtil::setFontWidget(m_pLabelText, getDisplaySize(1).toInt(), false);
}

void FPanelSplitButton::updateIcon()
{
    QIcon icon;
    if (m_bKeep)
        icon = QIcon(m_strIcon);
    else
        icon = FUtil::changeIconColor(m_strIcon, getColorTheme(3));
    m_pLabelIcon->setPixmap(icon.pixmap(getDisplaySize(2).toInt(), getDisplaySize(2).toInt()));
}