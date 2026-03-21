#include <FancyWidgets/FGroupBox.h>
#include <FancyWidgets/FUtil.h>
#include <QPainter>
#include <QApplication>
#include <QStyleOptionGroupBox>
#include <QPainterPath>

FGroupBox::FGroupBox(QWidget *parent)
    : FGroupBox(QString(""), parent)
{
}

FGroupBox::FGroupBox(const QString &title, QWidget *parent)
    : QGroupBox(title, parent), FThemeableWidget()
{
    // Suppress native QGroupBox drawing entirely
    setStyleSheet("QGroupBox { border: none; background: transparent; }");

    // Listen to global focus changes
    connect(qApp, &QApplication::focusChanged, this, &FGroupBox::onFocusChanged);

    SET_UP_THEME(FGroupBox)
}

FGroupBox::~FGroupBox()
{
    // Important: disconnect to avoid dangling callbacks if the widget dies
    disconnect(qApp, &QApplication::focusChanged, this, &FGroupBox::onFocusChanged);
}

void FGroupBox::applyTheme()
{
    FUtil::setFontWidget(this, getDisplaySize(1).toInt(), false);
    
    // We intentionally set margin-top in the stylesheet so child widgets
    // are automatically pushed down, leaving space for our custom title bar.
    // The margin equals the title text height plus some padding.
    const int padding = getDisplaySize(2).toInt();
    QFontMetrics fm(this->font());
    const int titleHeight = fm.height() + padding * 2;

    setStyleSheet(QString("QGroupBox { border: none; background: transparent; margin-top: %1px; }").arg(titleHeight));
}

QColor FGroupBox::getColorTheme(int idx)
{
    SET_UP_COLOR(0, Default, backgroundSection1); // Normal border & title background
    SET_UP_COLOR(1, Default, dominant);           // Focused border & title background
    SET_UP_COLOR(2, Default, transparent);        // Content background (or backgroundSection2 if preferred)
    SET_UP_COLOR(3, Default, textNormal);         // Title text color

    return QColor(qRgb(0, 0, 0));
}

QVariant FGroupBox::getDisplaySize(int idx)
{
    SET_UP_DISPLAY_SIZE(0, Primary, filletSize1);    // corner radius
    SET_UP_DISPLAY_SIZE(1, Primary, fontNormalSize); // title font size
    SET_UP_DISPLAY_SIZE(2, Primary, offsetSize1);    // title padding

    return QVariant(0);
}

void FGroupBox::onFocusChanged(QWidget *old, QWidget *now)
{
    Q_UNUSED(old);
    
    // If the widget gaining focus is this group box or one of its descendants, light up
    bool isNowFocused = (now && this->isAncestorOf(now));

    if (m_bFocused != isNowFocused)
    {
        m_bFocused = isNowFocused;
        update();
    }
}

void FGroupBox::paintEvent(QPaintEvent * /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const int radius = getDisplaySize(0).toInt();
    const int padding = getDisplaySize(2).toInt();
    
    QFontMetrics fm(this->font());
    const int titleHeight = fm.height() + padding * 2;
    
    QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    
    // Colors
    QColor themeColor = m_bFocused ? getColorTheme(1) : getColorTheme(0);
    QColor contentBg = getColorTheme(2);

    // 1. Draw content background using FUtil
    // FUtil::fillRoundedRect(&painter, r, contentBg, radius, radius, radius, radius);

    // 2. Draw border
    FUtil::drawRoundedRect(&painter, r, 1.0, themeColor, radius, radius, radius, radius);

    // 3. Draw colored title bar.
    // Since FUtil::fillRoundedRect fills the whole rect, we draw a path for the top part manually
    // or clip and fill.
    QRectF titleRect(r.left(), r.top(), r.width(), titleHeight);
    QPainterPath titlePath;
    titlePath.setFillRule(Qt::WindingFill);
    titlePath.addRoundedRect(titleRect, radius, radius);
    
    // We only want the top to be rounded. So we add a solid rectangle over the bottom rounded corners.
    QRectF bottomHalf(r.left(), titleRect.bottom() - radius, r.width(), radius);
    titlePath.addRect(bottomHalf);
    
    painter.fillPath(titlePath.simplified(), themeColor);

    // 4. Draw a bottom border for the title bar if necessary, but the border
    // is already drawn. We just need to make sure the border doesn't look cut off.

    // 5. Draw Title Text
    painter.setPen(getColorTheme(3));
    QRect textRect(int(r.left()) + padding, int(r.top()), int(r.width()) - padding * 2, titleHeight);
    
    QString t = fontMetrics().elidedText(title(), Qt::ElideRight, textRect.width());
    painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, t);
}
