#include <FancyWidgets/FPushButton.h>
#include <FancyWidgets/FUtil.h>
#include <QMouseEvent>
#include <QFocusEvent>
#include <QStyleOption>
#include <QPainterPath>

FPushButton::FPushButton(QWidget *parent)
    : FPushButton(QString(""), parent)
{
}

FPushButton::FPushButton(const QString &text, QWidget *parent)
    : QPushButton(text, parent), FThemeableWidget()
{
    setAttribute(Qt::WA_Hover, true);
    setFocusPolicy(Qt::StrongFocus);
    setCursor(Qt::PointingHandCursor);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    SET_UP_THEME(FPushButton)
}

void FPushButton::applyTheme()
{
    // Suppress native QPushButton rendering so our paintEvent has full control
    setStyleSheet(QString("QPushButton { background: transparent; border: none; padding-left: %1px;}").arg(QString::number(getDisplaySize(2).toInt())));
    FUtil::setFontWidget(this, getDisplaySize(1).toInt(), false);
    update();
}

QColor FPushButton::getColorTheme(int idx)
{
    SET_UP_COLOR(0, Default, widgetNormal);       // normal background
    SET_UP_COLOR(1, Default, widgetHover);        // hover background
    SET_UP_COLOR(2, Default, widgetCheck);        // pressed background
    SET_UP_COLOR(3, Default, dominant);           // focus border / accent
    SET_UP_COLOR(4, Default, textHeader1);        // button label text
    SET_UP_COLOR(5, Default, transparent);        // fully transparent
    SET_UP_COLOR(6, Default, backgroundSection1); // normal border

    return QColor(qRgb(0, 0, 0));
}

QVariant FPushButton::getDisplaySize(int idx)
{
    SET_UP_DISPLAY_SIZE(0, Primary, filletSize1);    // corner radius (6 px)
    SET_UP_DISPLAY_SIZE(1, Primary, fontNormalSize); // text font size
    SET_UP_DISPLAY_SIZE(2, Primary, offsetSize2);

    return QVariant(0);
}

void FPushButton::paintEvent(QPaintEvent * /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const qreal radius   = getDisplaySize(0).toInt();
    const QRectF r       = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);

    // --- Fill ---
    QColor fillColor;
    if (m_bPressed)
        fillColor = getColorTheme(2);
    else if (m_bHover)
        fillColor = getColorTheme(1);
    else
        fillColor = getColorTheme(0);

    FUtil::fillRoundedRect(&painter, r, fillColor, radius, radius, radius, radius);

    // --- Border ---
    // Subtle 1 px border
    FUtil::drawRoundedRect(&painter, r, 1.0, getColorTheme(6), radius, radius, radius, radius);

    // --- Text ---
    painter.setPen(getColorTheme(4));
    painter.drawText(rect(), Qt::AlignCenter, text());
}

void FPushButton::enterEvent(QEnterEvent *event)
{
    m_bHover = true;
    update();
    QPushButton::enterEvent(event);
}

void FPushButton::leaveEvent(QEvent *event)
{
    m_bHover = false;
    update();
    QPushButton::leaveEvent(event);
}

void FPushButton::mousePressEvent(QMouseEvent *event)
{
    m_bPressed = true;
    update();
    QPushButton::mousePressEvent(event);
}

void FPushButton::mouseReleaseEvent(QMouseEvent *event)
{
    m_bPressed = false;
    update();
    QPushButton::mouseReleaseEvent(event);
}

void FPushButton::focusInEvent(QFocusEvent *event)
{
    m_bFocused = true;
    update();
    QPushButton::focusInEvent(event);
}

void FPushButton::focusOutEvent(QFocusEvent *event)
{
    m_bFocused = false;
    update();
    QPushButton::focusOutEvent(event);
}
