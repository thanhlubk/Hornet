#include <FancyWidgets/FArrowButton.h>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QPropertyAnimation>
#include <QMouseEvent>
#include <FancyWidgets/FUtil.h>
#include <QStyleOption>

#define ROTATE_ANGLE 45
#define ANIMATION_MOVE_DISTANCE 10

FArrowButton::FArrowButton(const QString& text, const QString& leftIcon, const QString& arrowIcon, bool keep, QWidget* parent)
    : QWidget(parent), FThemeableWidget(), m_strIconMain(leftIcon), m_strIconArrow(arrowIcon), m_bKeep(keep), m_iRotation(0), m_bExpanded(false), m_bHover(false), m_bAnimMove(false), m_bAnimRotate(true), m_bBold(true)
{
    m_pWidgetSpacer = nullptr;
    m_pLabelMainIcon = nullptr;
    m_pLabelText = nullptr;
    m_pLabelArrow = nullptr;
    m_pLayout = nullptr;
    m_pAnimRotate = nullptr;
    m_pAnimMove = nullptr;

    setCursor(Qt::PointingHandCursor);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    
    m_pLabelText = new QLabel(text, this);

    m_pLayout = new QHBoxLayout(this);
    m_pLayout->setContentsMargins(0, 0, 0, 0);
    m_pLayout->setSpacing(0);
    m_pLayout->addWidget(m_pLabelText);
    m_pLayout->addStretch();

    if (m_bAnimMove)
        initSpacer();
    if (!m_strIconMain.isEmpty())
        initMainIcon();
    if (!m_strIconArrow.isEmpty())
        initArrow();

    SET_UP_THEME(FArrowButton)
}

void FArrowButton::applyTheme() 
{
    updateStyle();
}

QColor FArrowButton::getColorTheme(int idx)
{
    SET_UP_COLOR(0, Default, backgroundSection1);
    SET_UP_COLOR(1, Default, widgetHover);
    SET_UP_COLOR(2, Default, textNormal);
    SET_UP_COLOR(3, Default, dominant);
    SET_UP_COLOR(4, Default, transparent);

    SET_UP_COLOR(0, Custom1, backgroundSection2);
    SET_UP_COLOR(1, Custom1, backgroundSection1);
    SET_UP_COLOR(2, Custom1, textNormal);
    SET_UP_COLOR(3, Custom1, dominant);
    SET_UP_COLOR(4, Custom1, transparent);

    return QColor(qRgb(0, 0, 0));
}

QVariant FArrowButton::getDisplaySize(int idx)
{
    SET_UP_DISPLAY_SIZE(0, Primary, filletSize1);
    SET_UP_DISPLAY_SIZE(1, Primary, fontNormalSize);
    SET_UP_DISPLAY_SIZE(2, Primary, iconSize1);
    SET_UP_DISPLAY_SIZE(3, Primary, iconSize3);

    return QVariant(0);
}

void FArrowButton::mousePressEvent(QMouseEvent* event) 
{
    m_bExpanded = !m_bExpanded;

    if (m_bAnimRotate && m_pAnimRotate)
        m_pAnimRotate->setDirection(m_bExpanded ? QAbstractAnimation::Forward : QAbstractAnimation::Backward);
    
    if (m_bAnimRotate && m_pAnimRotate)
        m_pAnimRotate->start();
    
    emit arrowToggled(m_bExpanded);
    emit clicked();
    QWidget::mousePressEvent(event);
}

void FArrowButton::setRotation(int angle) {
    m_iRotation = angle;

    if (m_strIconArrow.isEmpty() || !m_pLabelArrow)
        return;

    // auto icon = FUtil::changeIconColor(m_strIconArrow, getColorTheme(3));
    QPixmap original = m_iconArrow.pixmap(getDisplaySize(3).toInt(), getDisplaySize(3).toInt()); // a right arrow icon
    if (m_iRotation != 0)
    {
        QPixmap rotated(original.size());
        rotated.fill(Qt::transparent);

        QPainter painter(&rotated);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        painter.translate(original.width() / 2.0, original.height() / 2.0);
        if (m_bAnimRotate)
            painter.rotate(angle);
        
        painter.translate(-original.width() / 2.0, -original.height() / 2.0);

        painter.drawPixmap(0, 0, original);
        painter.end();

        original = rotated;
    }

    m_pLabelArrow->setPixmap(original);
}

void FArrowButton::enterEvent(QEnterEvent* event) 
{
    if (m_bAnimMove && m_pAnimMove)
    {
        m_pAnimMove->setDirection(QAbstractAnimation::Forward);
        animateMove();
    }

    m_bHover = true;
    QWidget::enterEvent(event);
    updateStyle();
}

void FArrowButton::leaveEvent(QEvent* event) 
{
    if (m_bAnimMove && m_pAnimMove)
    {
        m_pAnimMove->setDirection(QAbstractAnimation::Backward);
        animateMove();
    }

    m_bHover = false;
    QWidget::leaveEvent(event);
    updateStyle();
}

void FArrowButton::paintEvent(QPaintEvent* event) {
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void FArrowButton::animateMove() 
{
    if (m_bAnimMove && m_pAnimMove)
    {
        if (m_pAnimMove->state() == QAbstractAnimation::Running)
            m_pAnimMove->stop();

        m_pAnimMove->start();
    }
}

void FArrowButton::updateStyle()
{
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

        m_pLabelMainIcon->setPixmap(icon.pixmap(getDisplaySize(2).toInt(), getDisplaySize(2).toInt()));
    }
    
    if (m_pLabelArrow && !m_strIconArrow.isEmpty())
    {
        m_pLabelArrow->setStyleSheet(QString("background: %1;").arg(getColorTheme(4).name(QColor::HexArgb)));
        m_iconArrow = FUtil::changeIconColor(m_strIconArrow, getColorTheme(3));
        setRotation(m_iRotation);
    }

    if (!m_bHover)
    {
        if (m_bAnimMove && m_pAnimMove)
        {
            m_pAnimMove->setDirection(QAbstractAnimation::Backward);
            animateMove();
        }
    }
}

bool FArrowButton::isExpand() const
{
    return m_bExpanded;
}

void FArrowButton::enableMoveAnimation(bool enable)
{
    m_bAnimMove = enable;
    initSpacer();
}

void FArrowButton::enableRotateAnimation(bool enable)
{
    m_bAnimRotate = enable;
    initArrow();
}

void FArrowButton::enableBold(bool enable)
{
    m_bBold = enable;
    updateStyleText();
}

void FArrowButton::initSpacer()
{
    if (!m_pWidgetSpacer && !m_pAnimMove && m_bAnimMove)
    {
        m_pWidgetSpacer = new QWidget(this);
        m_pWidgetSpacer->setFixedWidth(0);  // Start from 0
        m_pWidgetSpacer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

        m_pAnimMove = new QPropertyAnimation(m_pWidgetSpacer, "minimumWidth", this);
        m_pAnimMove->setDuration(200);
        m_pAnimMove->setStartValue(0);
        m_pAnimMove->setEndValue(ANIMATION_MOVE_DISTANCE);
        m_pAnimMove->setEasingCurve(QEasingCurve::OutQuad);

        m_pLayout->insertWidget(0, m_pWidgetSpacer);
    }
}

void FArrowButton::initMainIcon()
{
    if (m_strIconMain.isEmpty())
        return;
    
    if (!m_pLabelMainIcon && m_pLabelText)
    {
        m_pLabelMainIcon = new QLabel(this);
        int index = m_pLayout->indexOf(m_pLabelText) - 1;
        m_pLayout->insertWidget(index, m_pLabelMainIcon);
    }
}

void FArrowButton::initArrow()
{
    if (m_strIconArrow.isEmpty())
        return;

    if (!m_pLabelArrow && m_pLabelText)
    {
        m_pLabelArrow = new QLabel(this);
        int index = m_pLayout->indexOf(m_pLabelText) + 2;
        m_pLayout->insertWidget(index, m_pLabelArrow);
    }
    

    if (!m_pAnimRotate && m_bAnimRotate)
    {
        m_pAnimRotate = new QPropertyAnimation(this, "rotation");
        m_pAnimRotate->setDuration(200);
        m_pAnimRotate->setStartValue(0);
        m_pAnimRotate->setEndValue(ROTATE_ANGLE);
    }
}

void FArrowButton::updateStyleText()
{
    m_pLabelText->setStyleSheet(QString("background: %1; color: %2;").arg(getColorTheme(4).name(QColor::HexArgb), getColorTheme(2).name()));
    FUtil::setFontWidget(m_pLabelText, getDisplaySize(1).toInt(), m_bBold);
}