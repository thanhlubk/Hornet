#include <FancyWidgets/FSideBarTab.h>
#include <FancyWidgets/FUtil.h>
#include <QStyleOption>
#include <QPainter>

FSideBarTab::FSideBarTab(QWidget* parent)
    : QWidget(parent), FThemeableWidget(), m_bCheck(false), m_strTitle(""), m_bKeepIcon(true)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setCursor(Qt::PointingHandCursor);
    setFixedWidth(parent->width());

    m_pIconLabel = new QLabel(this);
    m_pIconLabel->setAlignment(Qt::AlignCenter);

    m_pTextLabel = new QLabel(m_strTitle, this);
    m_pTextLabel->setAlignment(Qt::AlignCenter);
    
    m_pFrame = new QFrame(this);

    updateIconAndStyle(false);
    updateBackground(true);

    SET_UP_THEME(FSideBarTab)
}

void FSideBarTab::applyTheme() {
    if (!m_bKeepIcon)
    {
        m_iconNormal = FUtil::changeIconColor(m_iconNormal, getColorTheme(2));
        m_iconHover = FUtil::changeIconColor(m_iconHover, getColorTheme(3));
        m_iconChecked = FUtil::changeIconColor(m_iconChecked, getColorTheme(4));
    }
    updateIconAndStyle(isChecked());
    updateBackground(false);

    setFixedHeight(getDisplaySize(4).toInt());
	FUtil::setFontWidget(m_pTextLabel, getDisplaySize(1).toInt(), false);
    m_pIconLabel->setFixedSize(getDisplaySize(5).toInt(), getDisplaySize(4).toInt());
    m_pTextLabel->setFixedSize(getDisplaySize(5).toInt(), getDisplaySize(4).toInt());
}

QColor FSideBarTab::getColorTheme(int idx)
{
    SET_UP_COLOR(0, Default, backgroundApp);
    SET_UP_COLOR(1, Default, backgroundSection1);
    SET_UP_COLOR(2, Default, textNormal);
    SET_UP_COLOR(3, Default, textHeader1);
    SET_UP_COLOR(4, Default, dominant);

    return QColor(qRgb(0, 0, 0));
}

QVariant FSideBarTab::getDisplaySize(int idx)
{
    SET_UP_DISPLAY_SIZE(0, Primary, iconSize1);
    SET_UP_DISPLAY_SIZE(1, Primary, fontSubSize);
    SET_UP_DISPLAY_SIZE(2, Primary, offsetSize1);
    SET_UP_DISPLAY_SIZE(3, Primary, filletSize1);
    SET_UP_DISPLAY_SIZE(4, Primary, sidebarTabHeight);
    SET_UP_DISPLAY_SIZE(5, Primary, sidebarTabWidth);

    return QVariant(0);
}

void FSideBarTab::paintEvent(QPaintEvent* event)
{
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

    if (m_bCheck)
    {
        QPen pen(getColorTheme(4), 4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        QBrush brush(getColorTheme(4), Qt::SolidPattern);

        QPainter painter(this);
        painter.setPen(pen);
        painter.setBrush(brush);

        painter.setRenderHint(QPainter::Antialiasing, true);

        QPoint point1{ 2, getDisplaySize(4).toInt()/2 - 11 };
        QPoint point2{ 2, getDisplaySize(4).toInt()/2 + 11 };
        painter.drawLine(point1, point2);
        painter.save();
        painter.restore(); // Restore state
        painter.end();
    }
}

void FSideBarTab::setIcon(const QIcon& iconNormal, const QIcon& iconChecked)
{
    m_iconNormal = iconNormal;
    m_iconHover = iconNormal;
    m_iconChecked = iconChecked;
    m_bKeepIcon = true;

    updateIconAndStyle(isChecked());
}

void FSideBarTab::setIcon(const QString& iconNormalPath, const QString& iconCheckedPath)
{
    m_iconNormal = FUtil::changeIconColor(iconNormalPath, getColorTheme(2));
    m_iconHover = FUtil::changeIconColor(iconNormalPath, getColorTheme(3));
    m_iconChecked = FUtil::changeIconColor(iconCheckedPath, getColorTheme(4));
    m_bKeepIcon = false;

    updateIconAndStyle(isChecked());
}

void FSideBarTab::updateIconAndStyle(bool active)
{
    QIcon icon = active ? m_iconHover : m_iconNormal;
    if (isChecked())
    {
        m_pFrame->setStyleSheet(QString("background: %1; border-radius: 0px;").arg(getColorTheme(1).name()));
        icon = m_iconChecked;
    }
    else
        m_pFrame->setStyleSheet("background: transparent; border-radius: 0px;");
    
    QPixmap pix = icon.pixmap(getDisplaySize(0).toInt(), getDisplaySize(0).toInt()); 
    m_pIconLabel->setPixmap(pix);

    if (isChecked())
    {
        m_pTextLabel->setFixedHeight(0);
        m_pTextLabel->move(0, 0);
        m_pTextLabel->raise();

        m_pIconLabel->move(0, 0);
        m_pIconLabel->raise();
    }
    else
    {
        m_pTextLabel->setFixedHeight(getDisplaySize(4).toInt());
        m_pTextLabel->move(0, getDisplaySize(4).toInt()/2 - getDisplaySize(1).toInt()*2);
        m_pTextLabel->raise();

        m_pIconLabel->move(0, -getDisplaySize(1).toInt());
        m_pIconLabel->raise();
    }
}

void FSideBarTab::updateBackground(bool active)
{
    if (m_bCheck)
        active = true;

    setStyleSheet(QString("QWidget { background: %4; border-radius: %2px; margin-right: %3px;} QLabel { background: transparent; color: %1; border-radius: %2px;}").arg(active ? getColorTheme(3).name(): getColorTheme(2).name(), QString::number(getDisplaySize(3).toInt()), QString::number(getDisplaySize(2).toInt()), active ? getColorTheme(1).name() : getColorTheme(0).name()));

}

void FSideBarTab::enterEvent(QEnterEvent* event)
{
    QWidget::enterEvent(event);
    updateIconAndStyle(true);
    updateBackground(true);
}

void FSideBarTab::leaveEvent(QEvent* event)
{
    QWidget::leaveEvent(event);
    updateIconAndStyle(false);
    updateBackground(false);
}

void FSideBarTab::mousePressEvent(QMouseEvent* event)
{
    QWidget::mousePressEvent(event);

    if (!isChecked())
    {
        setChecked(!isChecked());
        emit clicked();
    }
}

void FSideBarTab::mouseReleaseEvent(QMouseEvent* event)
{
    QWidget::mouseReleaseEvent(event);
}

void FSideBarTab::setChecked(bool checked)
{
    m_bCheck = checked;
    
    updateIconAndStyle(checked);
    updateBackground(checked);
}

bool FSideBarTab::isChecked() const
{
    return m_bCheck;
}

void FSideBarTab::setTitle(const QString& title)
{
    m_strTitle = title;
    m_pTextLabel->setText(m_strTitle);
}

QString FSideBarTab::title() const
{
    return m_strTitle;
}

void FSideBarTab::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);

    if (m_pFrame)
    {
        m_pFrame->setFixedSize(width() + getDisplaySize(2).toInt(), height());
        m_pFrame->move(width()/2 + getDisplaySize(2).toInt(), 0);
        m_pFrame->lower();
    }
}
