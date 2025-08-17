#include <FancyWidgets/FTreeItem.h>
#include <QMouseEvent>
#include <QStyleOption>
#include <QPainter>
#include <FancyWidgets/FUtil.h>

FTreeItem::FTreeItem(const QString &title, const QString &icon, QWidget *parent)
    : QWidget(parent), FThemeableWidget(), m_strIconMain(icon), m_bKeep(false), m_bHover(false), m_bBold(false)
{
    m_pLabelMainIcon = nullptr;
    m_pLabelText = nullptr;
    m_pCheckBox = nullptr;

    setMouseTracking(true);
    setAttribute(Qt::WA_Hover);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    m_pLabelText = new QLabel(title, this);
    m_pCheckBox = new FCheckBox(this);

    m_pLayout = new QHBoxLayout(this);

    m_pLayout->addWidget(m_pLabelText);
    m_pLayout->addStretch();
    m_pLayout->addWidget(m_pCheckBox);

    if (!m_strIconMain.isEmpty())
        initMainIcon();

    SET_UP_THEME(FTreeItem)
}

void FTreeItem::applyTheme()
{
    updateStyle();
}

QColor FTreeItem::getColorTheme(int idx)
{
    SET_UP_COLOR(0, Default, backgroundSection1);
    SET_UP_COLOR(1, Default, widgetHover);
    SET_UP_COLOR(2, Default, textNormal);
    SET_UP_COLOR(3, Default, dominant);
    SET_UP_COLOR(4, Default, transparent);

    return QColor(qRgb(0, 0, 0));
}

QVariant FTreeItem::getDisplaySize(int idx)
{
    SET_UP_DISPLAY_SIZE(0, Primary, filletSize1);
    SET_UP_DISPLAY_SIZE(1, Primary, fontNormalSize);
    SET_UP_DISPLAY_SIZE(2, Primary, offsetSize1);
    SET_UP_DISPLAY_SIZE(3, Primary, iconSize3);

    return QVariant(0);
}

void FTreeItem::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        emit itemClicked();
    }
    QWidget::mousePressEvent(event);
}

void FTreeItem::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        emit itemDoubleClicked();
    }
    QWidget::mouseDoubleClickEvent(event);
}

void FTreeItem::enterEvent(QEnterEvent *event)
{
    m_bHover = true;
    QWidget::enterEvent(event);
    updateStyle();
}

void FTreeItem::leaveEvent(QEvent *event)
{
    m_bHover = false;
    QWidget::leaveEvent(event);
    updateStyle();
}

void FTreeItem::paintEvent(QPaintEvent *event)
{
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void FTreeItem::enableBold(bool enable)
{
    m_bBold = enable;
    updateStyleText();
}

void FTreeItem::initMainIcon()
{
    if (m_strIconMain.isEmpty())
        return;

    if (!m_pLabelMainIcon && m_pLabelText)
    {
        m_pLabelMainIcon = new QLabel(this);
        int index = m_pLayout->indexOf(m_pLabelText);
        m_pLayout->insertWidget(index, m_pLabelMainIcon);
    }
}

void FTreeItem::updateStyleText()
{
    m_pLabelText->setStyleSheet(QString("background: %1; color: %2;").arg(getColorTheme(4).name(QColor::HexArgb), getColorTheme(2).name()));
    FUtil::setFontWidget(m_pLabelText, getDisplaySize(1).toInt(), m_bBold);
}

void FTreeItem::updateStyle()
{
    m_pLayout->setContentsMargins(getDisplaySize(2).toInt(), 0, getDisplaySize(2).toInt(), 0);

    setStyleSheet(QString("QWidget { background: %1; border-radius: %2px; }").arg(m_bHover ? getColorTheme(1).name() : getColorTheme(0).name(), QString::number(getDisplaySize(0).toInt())));

    updateStyleText();

    if (m_pLabelMainIcon && !m_strIconMain.isEmpty())
    {
        m_pLabelMainIcon->setStyleSheet(QString("background: %1;").arg(getColorTheme(4).name(QColor::HexArgb)));
        QIcon icon;
        if (m_bKeep)
            icon = FUtil::changeIconColor(m_strIconMain, getColorTheme(3));
        else
            icon = QIcon(m_strIconMain);

        m_pLabelMainIcon->setPixmap(icon.pixmap(getDisplaySize(3).toInt(), getDisplaySize(3).toInt()));
    }
}