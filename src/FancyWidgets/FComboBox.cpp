#include <FancyWidgets/FComboBox.h>
#include <FancyWidgets/FUtil.h>
#include <QApplication>
#include <QFrame>
#include <QHideEvent>
#include <QHash>
#include <QItemSelectionModel>
#include <QListView>
#include <QModelIndex>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPersistentModelIndex>
#include <QShowEvent>
#include <QSignalBlocker>
#include <QStyle>
#include <QStylePainter>
#include <QStyledItemDelegate>
#include <QVariantAnimation>
#include <QtMath>
#include <QTimer>

class FComboBox::FComboBoxAnimationDelegate final : public QStyledItemDelegate
{
public:
    explicit FComboBoxAnimationDelegate(FComboBoxAnimationView* view);

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    FComboBoxAnimationView* m_view = nullptr;
};

class FComboBox::FComboBoxAnimationView final : public QListView
{
public:
    explicit FComboBoxAnimationView(int animMs = 250, qreal shiftPx = 20.0, QWidget* parent = nullptr);
    void setAnimationDuration(int ms) { m_animMs = ms; }
    void setShiftDistance(qreal px) { m_shiftPx = px; }

protected:
    void showEvent(QShowEvent* e) override;
    void hideEvent(QHideEvent* e) override;
    bool event(QEvent* e) override;
    bool eventFilter(QObject* obj, QEvent* e) override;
    void currentChanged(const QModelIndex& current, const QModelIndex& previous) override;

private:
    void setOffsetImmediate(const QPersistentModelIndex& idx, qreal x);
    void animateIndex(const QPersistentModelIndex& idx, qreal target);
    void clearAllState();
    void attachPopupContainer();
    void applyOpenState();

private:
    QWidget* m_popupContainer = nullptr;
    bool m_opening = false; // suppress animation while popup is opening

    QPersistentModelIndex m_focused;
    QHash<QPersistentModelIndex, qreal> m_offsets;
    QHash<QPersistentModelIndex, QVariantAnimation*> m_anims;

    int   m_animMs  = 250;
    qreal m_shiftPx = 20.0;

    friend class FComboBoxAnimationDelegate;
};

FComboBox::FComboBoxAnimationDelegate::FComboBoxAnimationDelegate(FComboBoxAnimationView* view)
    : QStyledItemDelegate(view)
    , m_view(view)
{
}

QSize FComboBox::FComboBoxAnimationDelegate::sizeHint(const QStyleOptionViewItem& option,
                                   const QModelIndex& index) const
{
    QSize s = QStyledItemDelegate::sizeHint(option, index);
    s.rwidth() += int(qCeil(m_view->m_shiftPx)) + 4;; // extra width so shifted text doesn't clip
    return s;
}

void FComboBox::FComboBoxAnimationDelegate::paint(QPainter* p,
                               const QStyleOptionViewItem& option,
                               const QModelIndex& index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    const qreal offset = m_view->m_offsets.value(QPersistentModelIndex(index), 0.0);

    // Save original text, prevent default draw from drawing it
    const QString text = opt.text;
    opt.text.clear();

    const QWidget* w = opt.widget;
    QStyle* style = w ? w->style() : QApplication::style();

    // Draw item (background, selection, icon...) without text
    style->drawControl(QStyle::CE_ItemViewItem, &opt, p, w);

    // Draw shifted text
    QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &opt, w);
    textRect.translate(int(qRound(offset)), 0);

    p->save();
    p->setFont(opt.font);

    const QColor penColor =
        (opt.state & QStyle::State_Selected)
            ? opt.palette.color(QPalette::HighlightedText)
            : opt.palette.color(QPalette::Text);

    p->setPen(penColor);

    const QString elided =
        opt.fontMetrics.elidedText(text, opt.textElideMode, textRect.width());

    p->drawText(textRect, opt.displayAlignment, elided);
    p->restore();
}

FComboBox::FComboBoxAnimationView::FComboBoxAnimationView(int animMs, qreal shiftPx, QWidget* parent)
    : QListView(parent)
    , m_animMs(animMs)
    , m_shiftPx(shiftPx)
{
    setMouseTracking(true);
    setItemDelegate(new FComboBoxAnimationDelegate(this));

    // Hover drives "focus" via currentIndex (your requested behavior)
    connect(this, &QListView::entered, this, [this](const QModelIndex& idx) {
        if (!idx.isValid())
            return;

        setCurrentIndex(idx);

        if (auto* sm = selectionModel())
            sm->setCurrentIndex(idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    });
}

void FComboBox::FComboBoxAnimationView::showEvent(QShowEvent* e)
{
    m_opening = true;
    clearAllState();
    QListView::showEvent(e);

    QModelIndex idx = currentIndex();
    if (idx.isValid())
    {
        // ensure the row is highlighted immediately
        if (auto* sm = selectionModel())
        {
            // optional: prevent selection signals during opening
            QSignalBlocker blocker(sm);
            sm->setCurrentIndex(idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        }

        // don't need if when enable animation when pop up
        // snap text position (no animation)
        // setOffsetImmediate(QPersistentModelIndex(idx), m_shiftPx);

        // optional: gives focus-rect consistency depending on style
        setFocus(Qt::PopupFocusReason);
    }

    m_opening = false;
}

void FComboBox::FComboBoxAnimationView::hideEvent(QHideEvent* e)
{
    QListView::hideEvent(e);
    clearAllState();
}

void FComboBox::FComboBoxAnimationView::currentChanged(const QModelIndex& current,
                                    const QModelIndex& previous)
{
    QListView::currentChanged(current, previous);

    if (current.isValid())
    {
        if (auto* sm = selectionModel())
            sm->select(current, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    }

    const QPersistentModelIndex pcur(current);
    const QPersistentModelIndex pprev(previous);

    // While opening: never animate; just snap to desired state.
    if (m_opening)
    {
        if (pprev.isValid())
            setOffsetImmediate(pprev, 0.0);
        if (pcur.isValid())
            setOffsetImmediate(pcur, m_shiftPx);

        m_focused = pcur;
        return;
    }

    // Normal behavior:
    // previous animates back to 0, current animates to 20.
    animateIndex(pprev, 0.0);
    animateIndex(pcur, m_shiftPx);

    m_focused = pcur;
}

void FComboBox::FComboBoxAnimationView::setOffsetImmediate(const QPersistentModelIndex& idx, qreal x)
{
    if (!idx.isValid())
        return;

    // stop existing anim for this index
    if (auto* old = m_anims.take(idx))
    {
        old->stop();
        old->deleteLater();
    }

    if (qFuzzyIsNull(x))
        m_offsets.remove(idx);
    else
        m_offsets[idx] = x;

    viewport()->update(visualRect(QModelIndex(idx)));
}

void FComboBox::FComboBoxAnimationView::animateIndex(const QPersistentModelIndex& idx, qreal target)
{
    if (!idx.isValid())
        return;

    // If there is a running animation, cancel it and use its current value as "start".
    qreal start = 0.0;
    if (auto* running = m_anims.take(idx))
    {
        const QVariant cv = running->currentValue();
        start = cv.isValid() ? cv.toReal() : m_offsets.value(idx, 0.0);

        running->stop();
        running->deleteLater();
    }
    else
    {
        start = m_offsets.value(idx, 0.0);
    }

    // IMPORTANT: record the start immediately so a "leave" right after "enter"
    // still has a non-empty state (no dependence on first valueChanged()).
    if (qFuzzyIsNull(start))
        m_offsets.remove(idx);
    else
        m_offsets[idx] = start;

    // If we're already at target, snap and repaint (also ensures any old anim is stopped).
    if (qFuzzyCompare(start, target))
    {
        if (qFuzzyIsNull(target))
            m_offsets.remove(idx);
        else
            m_offsets[idx] = target;

        viewport()->update(visualRect(QModelIndex(idx)));
        return;
    }

    auto* a = new QVariantAnimation(this);
    a->setDuration(m_animMs);
    a->setEasingCurve(QEasingCurve::OutCubic);
    a->setStartValue(start);
    a->setEndValue(target);

    connect(a, &QVariantAnimation::valueChanged, this, [this, idx](const QVariant& v) {
        m_offsets[idx] = v.toReal();
        viewport()->update(visualRect(QModelIndex(idx)));
    });

    connect(a, &QVariantAnimation::finished, this, [this, idx, target, a]() {
        // Snap to final, then repaint (helps avoid any "stale painted" look).
        if (qFuzzyIsNull(target))
            m_offsets.remove(idx);
        else
            m_offsets[idx] = target;

        m_anims.remove(idx);
        viewport()->update(visualRect(QModelIndex(idx)));
        a->deleteLater();
    });

    m_anims.insert(idx, a);
    a->start();
}

void FComboBox::FComboBoxAnimationView::clearAllState()
{
    // stop & delete all animations
    const auto animsCopy = m_anims;
    for (auto* a : animsCopy)
    {
        if (!a) continue;
        a->stop();
        a->deleteLater();
    }
    m_anims.clear();
    m_offsets.clear();
    m_focused = QPersistentModelIndex();
}

bool FComboBox::FComboBoxAnimationView::event(QEvent* e)
{
    // When the popup is constructed/attached, this helps us re-attach the container.
    if (e->type() == QEvent::ParentChange || e->type() == QEvent::ShowToParent) {
        QTimer::singleShot(0, this, [this]{ attachPopupContainer(); });
    }
    return QListView::event(e);
}

void FComboBox::FComboBoxAnimationView::attachPopupContainer()
{
    // When shown as a combo popup, the view’s "window()" is the popup container (Qt::Popup).
    QWidget* c = this->window();
    if (!c || c == m_popupContainer) return;

    if (m_popupContainer) m_popupContainer->removeEventFilter(this);
    m_popupContainer = c;
    m_popupContainer->installEventFilter(this);
}

bool FComboBox::FComboBoxAnimationView::eventFilter(QObject* obj, QEvent* e)
{
    if (obj == m_popupContainer)
    {
        if (e->type() == QEvent::Hide || e->type() == QEvent::HideToParent)
        {
            // IMPORTANT: ensures each open starts from known state
            clearAllState();
        }
        else if (e->type() == QEvent::Show || e->type() == QEvent::ShowToParent)
        {
            // Choose ONE of these two (A or B). Comment the other.
            QTimer::singleShot(0, this, [this]{
                applyOpenState();
            });
        }
    }
    return QListView::eventFilter(obj, e);
}

void FComboBox::FComboBoxAnimationView::applyOpenState()
{
#if 1
    // A) animate on every open
    m_opening = true;

    // Ensure the row is highlighted immediately
    QModelIndex idx = currentIndex();
    if (idx.isValid() && selectionModel())
    {
        QSignalBlocker b(selectionModel());
        selectionModel()->setCurrentIndex(idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    }

    // Start from left (no animation)
    setOffsetImmediate(QPersistentModelIndex(idx), 0.0);
    m_focused = QPersistentModelIndex(idx);

    m_opening = false;

    // Then animate in to the right
    animateIndex(QPersistentModelIndex(idx), m_shiftPx);
#endif

#if 0
    // B) snap right (no animation)
    m_opening = true;

    QModelIndex idx = currentIndex();
    if (idx.isValid() && selectionModel())
    {
        QSignalBlocker b(selectionModel());
        selectionModel()->setCurrentIndex(idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    }

    // Directly snap to right (no animation)
    setOffsetImmediate(QPersistentModelIndex(idx), m_shiftPx);
    m_focused = QPersistentModelIndex(idx);

    m_opening = false;
#endif
}

FComboBox::FComboBox(QWidget* parent)
    : QComboBox(parent), FThemeableWidget(), m_iOffsetPopup(20)
{
    m_bPopupExpand = false;
    setMaxVisibleItems(10);
    // this->setView(new FComboBox::FComboBoxAnimationView(200, 10.0, this));
    // this->setView(new HoverSlideView(this));
    SET_UP_THEME(FComboBox)

    this->setView(new FComboBox::FComboBoxAnimationView(200, 10.0, this));

}

FComboBox::~FComboBox()
{
}

void FComboBox::applyTheme() 
{
    auto arrowUp = FUtil::getIconColorPath(":/FancyWidgets/res/icon/small/downward.png", getColorTheme(0));
    auto arrowDown = FUtil::getIconColorPath(":/FancyWidgets/res/icon/small/forward.png", getColorTheme(0));

    QString strComboboxStyle = QString("QComboBox { border: none; background: %1; color: %2; border-radius: %3px; padding-left: %7px; }\n"
    "QComboBox:hover { background: %4;} \n"
    "QComboBox:focus { border: %8px solid %4; } QComboBox:on { border: %8px solid %4;}\n"
	"/* Drop-down button area */ QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: center right; width: %10px; background: transparent; }\n"
	"/* Arrow */ QComboBox::down-arrow { image: url(%6); width: %9px; height: %9px; }\n"
    "QComboBox::down-arrow:on { image: url(%5); width: %9px; height: %9px; } \n"
	"/* Popup list */QComboBox QAbstractItemView { background: %1; color: %2; border: %8px solid %4; border-radius: %3px; padding: %7px; outline: 0px; selection-background-color: %4; selection-color: %2;}").arg(getColorTheme(1).name(), getColorTheme(3).name(), QString::number(getDisplaySize(0).toInt()), getColorTheme(2).name(), arrowUp, arrowDown, QString::number(getDisplaySize(1).toInt()), QString::number(getDisplaySize(2).toInt()), QString::number(getDisplaySize(3).toInt()), QString::number(getDisplaySize(3).toInt() * 2));

    auto strScrollbarVerticalStyle = FUtil::getStyleScrollbarVertical1(6, 0, 0, 0, getColorTheme(3).name(), getColorTheme(3).name());

    if (!m_bPopupExpand)
        strComboboxStyle += QString("QComboBox { combobox-popup: 0; }");
    else
        strComboboxStyle += QString("QComboBox { combobox-popup: 1; }");

    setStyleSheet(strComboboxStyle + strScrollbarVerticalStyle);
}

QColor FComboBox::getColorTheme(int idx)
{
    SET_UP_COLOR(0, Default, dominant);
    SET_UP_COLOR(1, Default, backgroundSection1);
    SET_UP_COLOR(2, Default, backgroundSection2);
    SET_UP_COLOR(3, Default, textNormal);
    SET_UP_COLOR(4, Default, transparent);

    return QColor(qRgb(0, 0, 0));
}

QVariant FComboBox::getDisplaySize(int idx)
{
    SET_UP_DISPLAY_SIZE(0, Primary, filletSize1);
    SET_UP_DISPLAY_SIZE(1, Primary, offsetSize2);
    SET_UP_DISPLAY_SIZE(2, Primary, highlightWidth1);
    SET_UP_DISPLAY_SIZE(3, Primary, iconSize3);
    SET_UP_DISPLAY_SIZE(4, Primary, highlightWidth2);
    SET_UP_DISPLAY_SIZE(5, Primary, scrollbarWidth2);
    return QVariant(0);
}

void FComboBox::showPopup()
{
    m_popupOpen = true;
    update();                  // trigger repaint so bottom line appears

    QComboBox::showPopup();
    QWidget* popup = this->findChild<QFrame*>();
    QPoint globalPos = this->mapToGlobal(QPoint(0, 0));
    popup->move(globalPos.x(), globalPos.y() + this->height() + m_iOffsetPopup);
}

void FComboBox::paintEvent(QPaintEvent* e)
{
    // Let Qt draw the combobox normally (including your stylesheet)
    QComboBox::paintEvent(e);

    if (!m_popupOpen)          // NEW: only draw while popup is open
        return;

    if (getDisplaySize(4).toInt() <= 0)
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
    const qreal h = qMin<qreal>(getDisplaySize(4).toInt(), inner.height());
    QRectF stripe(inner.left(), inner.bottom() - h + 1.0, inner.width(), h);
    p.fillRect(stripe, getColorTheme(0));

    p.restore();
}

void FComboBox::hidePopup()
{
    QComboBox::hidePopup();
    m_popupOpen = false;
    update(); // trigger repaint so bottom line disappears
}