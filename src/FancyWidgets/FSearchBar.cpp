#include <FancyWidgets/FSearchBar.h>
#include <QStyle>
#include <QStyleOption>
#include <QPainter>
#include <FancyWidgets/FUtil.h>

FSearchBar::FSearchBar(QWidget *parent)
    : QWidget(parent), FThemeableWidget()
{
    // Set search icon (use Qt standard icon or custom pixmap)
    m_pLabelIcon = new QLabel(this);
    m_pLabelIcon->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    // Set placeholder text
    m_pEditSearch = new QLineEdit(this);
    m_pEditSearch->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

    // Style and layout
    m_pLayout = new QHBoxLayout(this);
    m_pLayout->setContentsMargins(0, 0, 0, 0);
    m_pLayout->addWidget(m_pLabelIcon);
    m_pLayout->addWidget(m_pEditSearch);
    setLayout(m_pLayout);

    SET_UP_THEME(FSearchBar)
}

void FSearchBar::applyTheme() 
{
    m_pEditSearch->setStyleSheet(QString("background: %1; border: none; color: %2").arg(getColorTheme(0).name(QColor::HexArgb), getColorTheme(3).name()));
    FUtil::setFontWidget(m_pEditSearch, getDisplaySize(3).toInt(), false);

    m_pLabelIcon->setStyleSheet(QString("background: %1;").arg(getColorTheme(0).name(QColor::HexArgb)));
    QIcon icon = FUtil::changeIconColor(":/FancyWidgets/res/icon/small/search.png", getColorTheme(2));
    m_pLabelIcon->setPixmap(icon.pixmap(getDisplaySize(0).toInt(), getDisplaySize(0).toInt()));
    m_pLabelIcon->setFixedWidth(getDisplaySize(0).toInt() + getDisplaySize(1).toInt()*2);

    m_pLayout->setSpacing(getDisplaySize(1).toInt());

    setStyleSheet(QString("background: %1; border-radius: %2px; margin-top: %3px; margin-bottom: %3px;").arg(getColorTheme(1).name(), QString::number(getDisplaySize(2).toInt()), QString::number(getDisplaySize(4).toInt())));
}

QColor FSearchBar::getColorTheme(int idx)
{
    SET_UP_COLOR(0, Default, transparent);
    SET_UP_COLOR(1, Default, backgroundSection2);
    SET_UP_COLOR(2, Default, textNormal);
    SET_UP_COLOR(3, Default, textSub);

    return QColor(qRgb(0, 0, 0));
}

QVariant FSearchBar::getDisplaySize(int idx)
{
    SET_UP_DISPLAY_SIZE(0, Primary, iconSize3);
    SET_UP_DISPLAY_SIZE(1, Primary, offsetSize2);
    SET_UP_DISPLAY_SIZE(2, Primary, filletSize1);
    SET_UP_DISPLAY_SIZE(3, Primary, fontNormalSize);

    SET_UP_DISPLAY_SIZE(0, Secondary, iconSize3);
    SET_UP_DISPLAY_SIZE(1, Secondary, offsetSize2);
    SET_UP_DISPLAY_SIZE(2, Secondary, filletSize1);
    SET_UP_DISPLAY_SIZE(3, Secondary, fontNormalSize);
    SET_UP_DISPLAY_SIZE(4, Secondary, offsetSize2);

    return QVariant(0);
}

void FSearchBar::paintEvent(QPaintEvent* event)
{
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void FSearchBar::setPlaceholderText(const QString &text)
{
    m_pEditSearch->setPlaceholderText(text);
}