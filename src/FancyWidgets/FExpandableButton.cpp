#include <FancyWidgets/FExpandableButton.h>

FExpandableButton::FExpandableButton(const QString& text, const QString& leftIcon, const QString& arrowIcon, bool keep, QWidget* parent)
    : FArrowButton(text, leftIcon, arrowIcon, keep, parent)
{
    m_pMenu = new FPopupMenu(this);
    m_pMenu->setPopupPosition(FPopupMenu::Right);
    enableBold(false);
    enableRotateAnimation(false);

    connect(this, &FExpandableButton::clicked, this, &FExpandableButton::customSlot);
    connect(this, &FExpandableButton::customSignal, m_pMenu, &FPopupMenu::appear);
    connect(m_pMenu, &FPopupMenu::popupHidden, [this]()
            { m_bHover = false; updateStyle(); });
}

void FExpandableButton::addWidget(QWidget* widget)
{
    widget->setParent(this);
    m_pMenu->addWidget(widget);
}

void FExpandableButton::customSlot()
{
    QMetaObject::invokeMethod(this, "customSignal", Qt::QueuedConnection);
}