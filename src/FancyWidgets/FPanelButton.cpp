#include <FancyWidgets/FPanelButton.h>
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

FPanelButton::FPanelButton(QWidget *parent)
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

    SET_UP_THEME(FPanelButton)
}

void FPanelButton::applyTheme()
{
    updateStyle();
    updateIcon();
}

QColor FPanelButton::getColorTheme(int idx)
{
    SET_UP_COLOR(0, Default, backgroundSection2);
    SET_UP_COLOR(1, Default, widgetCheck);
    SET_UP_COLOR(2, Default, textHeader2);
    SET_UP_COLOR(3, Default, textNormal);

    return QColor(qRgb(0, 0, 0));
}

QVariant FPanelButton::getDisplaySize(int idx)
{
    SET_UP_DISPLAY_SIZE(0, Primary, filletSize1);
    SET_UP_DISPLAY_SIZE(1, Primary, fontNormalSize);
    SET_UP_DISPLAY_SIZE(2, Primary, iconSize2);

    return QVariant(0);
}

void FPanelButton::setIcon(const QString &icon, bool keep)
{
    m_strIcon = icon;
    updateIcon();
}

void FPanelButton::setText(const QString &text)
{
    m_pLabelText->setText(text);
}

void FPanelButton::mousePressEvent(QMouseEvent *event)
{
    emit clicked();
    QWidget::mousePressEvent(event);
}

void FPanelButton::enterEvent(QEnterEvent *event)
{
    m_bHover = true;
    updateStyle();
    QWidget::enterEvent(event);
}

void FPanelButton::leaveEvent(QEvent *event)
{
    m_bHover = false;
    updateStyle();
    QWidget::leaveEvent(event);
}

void FPanelButton::mouseMoveEvent(QMouseEvent *event)
{
    updateStyle();
    QWidget::mouseMoveEvent(event);
}

void FPanelButton::paintEvent(QPaintEvent *event)
{
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void FPanelButton::updateStyle()
{
    setStyleSheet(QString("QWidget { background: %1; border-radius: %2px; } QLabel { border: none; color: %3}").arg(!m_bHover ? getColorTheme(0).name() : getColorTheme(1).name(), QString::number(getDisplaySize(0).toInt()), getColorTheme(2).name()));

    if (m_bHover)
    {
        m_pLabelIcon->setStyleSheet(QString("background-color: %1; border-top-left-radius: %2px; border-top-right-radius: %2px; border-bottom-left-radius: 0px; border-bottom-right-radius: 0px;").arg(getColorTheme(1).name(), QString::number(getDisplaySize(0).toInt())));
        m_pLabelText->setStyleSheet(QString("background-color: %1; border-top-left-radius: 0px; border-top-right-radius: 0px; border-bottom-left-radius: %2px; border-bottom-right-radius: %2px;").arg(getColorTheme(1).name(), QString::number(getDisplaySize(0).toInt())));
    }
    else
    {
        m_pLabelIcon->setStyleSheet(QString("background-color: %1;").arg(getColorTheme(0).name()));
        m_pLabelText->setStyleSheet(QString("background-color: %1;").arg(getColorTheme(0).name()));
    }

    FUtil::setFontWidget(m_pLabelText, getDisplaySize(1).toInt(), false);
}

void FPanelButton::updateIcon()
{
    QIcon icon;
    if (m_bKeep)
        icon = QIcon(m_strIcon);
    else
        icon = FUtil::changeIconColor(m_strIcon, getColorTheme(3));
    m_pLabelIcon->setPixmap(icon.pixmap(getDisplaySize(2).toInt(), getDisplaySize(2).toInt()));
}