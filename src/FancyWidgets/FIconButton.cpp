#include <FancyWidgets/FIconButton.h>
#include <FancyWidgets/FUtil.h>
#include <QMouseEvent>
#include <QFocusEvent>
#include <QStyleOption>
#include <QPainterPath>
#include <QIcon>

FIconButton::FIconButton(QWidget *parent)
    : FIconButton(QIcon(), parent)
{
}

FIconButton::FIconButton(const QIcon &icon, QWidget *parent)
    : QPushButton(icon, "", parent), FThemeableWidget()
{
    setAttribute(Qt::WA_Hover, true);
    setFocusPolicy(Qt::StrongFocus);
    setCursor(Qt::PointingHandCursor);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    SET_UP_THEME(FIconButton)
}

void FIconButton::applyTheme()
{
    // Suppress native QPushButton rendering so our paintEvent has full control
    setStyleSheet("QPushButton { background: transparent; border: none; }");
    update();
}

QColor FIconButton::getColorTheme(int idx)
{
    SET_UP_COLOR(0, Default, transparent);        // normal background
    SET_UP_COLOR(1, Default, widgetHover);        // hover background
    SET_UP_COLOR(2, Default, widgetCheck);        // pressed background
    SET_UP_COLOR(3, Default, dominant);           // focus border / accent
    SET_UP_COLOR(4, Default, transparent);        // padding (if we need to ignore)
    SET_UP_COLOR(5, Default, transparent);   // fully transparent (if transparent above isn't)
    SET_UP_COLOR(6, Default, backgroundSection1); // normal border? (often transparent for flat icons)

    return QColor(qRgb(0, 0, 0));
}

QVariant FIconButton::getDisplaySize(int idx)
{
    SET_UP_DISPLAY_SIZE(0, Primary, filletSize1);    // corner radius (6 px)

    return QVariant(0);
}

void FIconButton::paintEvent(QPaintEvent * /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const qreal radius = getDisplaySize(0).toInt();
    const QRectF r     = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);

    // --- Fill ---
    // For an icon button, it is usually transparent until hovered/pressed
    QColor fillColor;
    if (m_bPressed)
        fillColor = getColorTheme(2);
    else if (m_bHover)
        fillColor = getColorTheme(1);
    else
        fillColor = getColorTheme(0);

    // If it's transparent, we don't necessarily need to fill, but filling with transparent does nothing bad
    FUtil::fillRoundedRect(&painter, r, fillColor, radius, radius, radius, radius);

    // --- Border ---
    if (m_bFocused)
    {
        // Win11-style: 2 px dominant-color border
        FUtil::drawRoundedRect(&painter, r, 2.0, getColorTheme(3), radius, radius, radius, radius);
    }
    // Alternatively, draw a subtle border only on hover, or no border unless focused
    // A standard icon button in many themes has no visible border unless focused.

    // --- Icon ---
    if (!icon().isNull())
    {
        // Draw icon centered. Note that QIcon::paint takes an integer QRect
        // We might want to respect the button's iconSize() property.
        QSize actualIconSize = iconSize();
        // Fallback size if iconSize isn't set
        if (!actualIconSize.isValid() || actualIconSize.isEmpty()) {
            actualIconSize = QSize(16, 16);
        }

        // Center the icon horizontally and vertically
        QRect iconRect(0, 0, actualIconSize.width(), actualIconSize.height());
        iconRect.moveCenter(rect().center());

        // Determine icon mode and state based on button state (e.g. disabled)
        QIcon::Mode mode = isEnabled() ? QIcon::Normal : QIcon::Disabled;
        QIcon::State state = isChecked() ? QIcon::On : QIcon::Off;
        
        icon().paint(&painter, iconRect, Qt::AlignCenter, mode, state);
    }
}

void FIconButton::enterEvent(QEnterEvent *event)
{
    m_bHover = true;
    update();
    QPushButton::enterEvent(event);
}

void FIconButton::leaveEvent(QEvent *event)
{
    m_bHover = false;
    update();
    QPushButton::leaveEvent(event);
}

void FIconButton::mousePressEvent(QMouseEvent *event)
{
    m_bPressed = true;
    update();
    QPushButton::mousePressEvent(event);
}

void FIconButton::mouseReleaseEvent(QMouseEvent *event)
{
    m_bPressed = false;
    update();
    QPushButton::mouseReleaseEvent(event);
}

void FIconButton::focusInEvent(QFocusEvent *event)
{
    m_bFocused = true;
    update();
    QPushButton::focusInEvent(event);
}

void FIconButton::focusOutEvent(QFocusEvent *event)
{
    m_bFocused = false;
    update();
    QPushButton::focusOutEvent(event);
}
