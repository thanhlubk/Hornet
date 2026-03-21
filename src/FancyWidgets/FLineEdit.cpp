#include <FancyWidgets/FLineEdit.h>
#include <FancyWidgets/FUtil.h>
#include <QPainter>
#include <QPainterPath>
#include <QStyleOption>
#include <QFocusEvent>
#include <QEvent>

FLineEdit::FLineEdit(QWidget *parent)
    : QWidget(parent), FThemeableWidget()
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    m_pEdit = new QLineEdit(this);
    m_pEdit->setFrame(false);
    m_pEdit->installEventFilter(this);

    m_pLayout = new QHBoxLayout(this);
    m_pLayout->setContentsMargins(0, 0, 0, 0);
    m_pLayout->setSpacing(0);
    m_pLayout->addWidget(m_pEdit);

    connect(m_pEdit, &QLineEdit::textChanged, this, &FLineEdit::textChanged);
    connect(m_pEdit, &QLineEdit::returnPressed, this, &FLineEdit::returnPressed);

    SET_UP_THEME(FLineEdit)
}

void FLineEdit::applyTheme()
{
    // Inner QLineEdit: transparent background, correct text color, no border
    m_pEdit->setStyleSheet(QString(
        "QLineEdit {"
        "   background: transparent;"
        "   border: none;"
        "   color: %1;"
        "}"
    ).arg(getColorTheme(2).name()));

    // Placeholder color via palette
    QPalette pal = m_pEdit->palette();
    pal.setColor(QPalette::PlaceholderText, getColorTheme(3));
    m_pEdit->setPalette(pal);

    FUtil::setFontWidget(m_pEdit, getDisplaySize(1).toInt(), false);

    update();
}

QColor FLineEdit::getColorTheme(int idx)
{
    SET_UP_COLOR(0, Default, backgroundSection2); // container fill
    SET_UP_COLOR(1, Default, dominant);           // bottom accent when focused
    SET_UP_COLOR(2, Default, textNormal);         // text color
    SET_UP_COLOR(3, Default, textSub);            // placeholder color
    SET_UP_COLOR(4, Default, widgetNormal);       // border color (unfocused)

    return QColor(qRgb(0, 0, 0));
}

QVariant FLineEdit::getDisplaySize(int idx)
{
    SET_UP_DISPLAY_SIZE(0, Primary, filletSize1);      // corner radius
    SET_UP_DISPLAY_SIZE(1, Primary, fontNormalSize);   // font size
    SET_UP_DISPLAY_SIZE(2, Primary, highlightWidth1);  // accent bar thickness
    SET_UP_DISPLAY_SIZE(3, Primary, highlightWidth2);

    return QVariant(0);
}

void FLineEdit::setText(const QString &text)
{
    m_pEdit->setText(text);
}

QString FLineEdit::text() const
{
    return m_pEdit->text();
}

void FLineEdit::setPlaceholderText(const QString &text)
{
    m_pEdit->setPlaceholderText(text);
}

void FLineEdit::setReadOnly(bool readOnly)
{
    m_pEdit->setReadOnly(readOnly);
}

void FLineEdit::paintEvent(QPaintEvent * /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const int   radius      = getDisplaySize(0).toInt();
    const int   accentH     = getDisplaySize(2).toInt();
    const QRectF r          = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);

    // -- Background fill with rounded corners --
    FUtil::fillRoundedRect(&painter, r, getColorTheme(0), radius, radius, radius, radius);

    // -- Border --
    FUtil::drawRoundedRect(&painter, r, 1.0, getColorTheme(4), radius, radius, radius, radius);

    // -- Bottom accent bar (Win11 underline on focus) --
    if (m_bFocused)
    {
        // const qreal accentY  = r.bottom() - accentH;
        // const QRectF accentR = QRectF(r.left() + radius, accentY, r.width() - radius * 2, accentH);

        // painter.setPen(Qt::NoPen);
        // painter.setBrush(getColorTheme(1));
        // painter.drawRect(accentR);

        if (getDisplaySize(3).toInt() <= 0)
            return;

        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);

        // Use an inner rect so we don't paint over the border
        QRectF outer = rect();
        outer.adjust(0.5, 0.5, -0.5, -0.5);

        int iBorderWidth = 0;
        QRectF inner = outer.adjusted(iBorderWidth, iBorderWidth, -iBorderWidth, -iBorderWidth);

        // Clip to rounded rect (so stripe is cut by border-radius)
        const qreal r = qMax<qreal>(0.0, getDisplaySize(0).toInt() - iBorderWidth);

        QPainterPath clip;
        clip.addRoundedRect(inner, r, r);

        p.save();
        p.setClipPath(clip);

        // Draw the bottom stripe INSIDE the clipped area
        const qreal h = qMin<qreal>(getDisplaySize(3).toInt(), inner.height());
        QRectF stripe(inner.left(), inner.bottom() - h + 1.0, inner.width(), h);
        p.fillRect(stripe, getColorTheme(1));

        p.restore();
    }
}

bool FLineEdit::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_pEdit)
    {
        if (event->type() == QEvent::FocusIn)
        {
            m_bFocused = true;
            update();
        }
        else if (event->type() == QEvent::FocusOut)
        {
            m_bFocused = false;
            update();
        }
    }
    return QWidget::eventFilter(watched, event);
}

