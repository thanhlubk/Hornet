#include <FancyWidgets/FViewHost.h>
#include <QVBoxLayout>
#include <QPainter>
#include <QEvent>

FViewHost::FViewHost(QWidget *parent)
    : QWidget(parent), m_pViewWidget(nullptr)
{
    auto *pLayout = new QVBoxLayout(this);
    pLayout->setContentsMargins(0, 0, 0, 0);
    setLayout(pLayout);

    // Optional: ensure we actually paint a background using palette / stylesheet
    setAttribute(Qt::WA_StyledBackground, true);
    // You can also do: setAutoFillBackground(true); if you prefer.
}

FViewHost::~FViewHost()
{
    // Detach the OpenGL widget but DO NOT delete it (Document owns it)
    if (m_pViewWidget)
    {
        layout()->removeWidget(m_pViewWidget);
        m_pViewWidget->hide();
        m_pViewWidget->setParent(nullptr);
        m_pViewWidget.clear(); // not strictly necessary but nice
    }
}

QWidget *FViewHost::currentViewWidget() const
{
    // QPointer<T> has operator T*() and data(), both fine
    return m_pViewWidget.data();
}

void FViewHost::setViewWidget(QWidget *pWidget)
{
    if (m_pViewWidget == pWidget)
        return;

    // Detach old one if still alive
    if (m_pViewWidget)
    {
        layout()->removeWidget(m_pViewWidget);
        m_pViewWidget->hide();
        m_pViewWidget->removeEventFilter(this);
        m_pViewWidget->setParent(nullptr);
        m_pViewWidget.clear();
    }

    m_pViewWidget = pWidget;

    if (!layout()) {
        auto* l = new QVBoxLayout(this);
        l->setContentsMargins(0,0,0,0);
        l->setSpacing(0);
    }

    // Attach new one if not null
    if (m_pViewWidget)
    {
        m_pViewWidget->setParent(this); // QWidget parenting for ownership/layout
        layout()->addWidget(m_pViewWidget);
        m_pViewWidget->installEventFilter(this);
        m_pViewWidget->show();
    }

    update(); // repaint host (e.g., to clear area if nullptr)
}

QWidget *FViewHost::takeViewWidget()
{
    QWidget *pWidget = m_pViewWidget.data();

    if (m_pViewWidget)
    {
        layout()->removeWidget(m_pViewWidget);
        m_pViewWidget->hide();
        m_pViewWidget->removeEventFilter(this);
        m_pViewWidget->setParent(nullptr);
        m_pViewWidget.clear();
    }

    update();
    return pWidget; // caller can reattach to another FViewHost
}

void FViewHost::paintEvent(QPaintEvent *event)
{
    // If we have a child QWidget, just let QWidget handle painting
    // (the child will cover the area anyway)
    if (m_pViewWidget)
    {
        QWidget::paintEvent(event);
        return;
    }

    // No OpenGL widget attached: paint a blank background (no crash, just empty)
    QPainter painter(this);

    // Option 1: use the standard window background from the palette
    painter.fillRect(rect(), palette().window());

    // Option 2 for transparent-style effect (if your window supports it):
    // painter.fillRect(rect(), Qt::transparent);

    Q_UNUSED(event);
}

bool FViewHost::eventFilter(QObject* obj, QEvent* ev)
{
    if (obj == m_pViewWidget) {
        if (ev->type() == QEvent::MouseButtonPress ||
            ev->type() == QEvent::FocusIn) {
            setActive(true);
        }
    }
    return QWidget::eventFilter(obj, ev);
}

void FViewHost::setActive(bool active)
{
    if (m_bIsActive == active)
        return;                 // IMPORTANT: no re-emit if unchanged

    m_bIsActive = active;

    if (active)
        emit activated();
    else
        emit deactivated();

    update();
}