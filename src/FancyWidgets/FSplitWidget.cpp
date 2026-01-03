#include <FancyWidgets/FSplitWidget.h>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSplitter>
// #include <QWidget>
#include <QSizePolicy>
#include <QSignalBlocker>
#include <QComboBox>
#include <QSet>

FSplitWidget::FSplitWidget(QWidget *parent)
    : QWidget(parent)
{
    createBlank();
}

QList<FSplitWidget*> FSplitWidget::leafNodes() const
{
    QList<FSplitWidget*> out;

    // Blank
    if (!m_pCentralWidget && !m_pControlWidget)
        return out;

    // Leaf (central is FViewHost)
    if (viewWidget())
    {
        out.push_back(const_cast<FSplitWidget*>(this));
        return out;
    }

    // Splitter node
    if (auto* sp = qobject_cast<QSplitter*>(m_pCentralWidget))
    {
        for (int i = 0; i < sp->count(); ++i)
        {
            if (auto* child = qobject_cast<FSplitWidget*>(sp->widget(i)))
                out += child->leafNodes();
        }
        return out;
    }

    // Promoted child stored as central
    if (auto* child = qobject_cast<FSplitWidget*>(m_pCentralWidget))
        return child->leafNodes();

    return out;
}

FSplitWidget* FSplitWidget::topMostParent() 
{
    FSplitWidget* top = this;
    while (true)
    {
        // Parent of a child node should be a QSplitter
        auto* sp = qobject_cast<QSplitter*>(top->parentWidget());
        if (!sp)
            break; // pattern broken -> stop

        // Parent of that QSplitter should be another FSplitWidget
        auto* ps = qobject_cast<FSplitWidget*>(sp->parentWidget());
        if (!ps)
            break; // pattern broken -> stop

        top = ps; // climb one level in the chain and continue
    }
    return top;
}

// ----------------------------------------------------------
// Initial blank state: only the main layout, no node yet
// ----------------------------------------------------------
void FSplitWidget::createBlank()
{
    m_pMainLayout = new QVBoxLayout(this);
    m_pMainLayout->setContentsMargins(0, 0, 0, 0);
    m_pMainLayout->setSpacing(0);

    m_pCentralWidget = nullptr;
    m_pControlWidget = nullptr;
}

bool FSplitWidget::isBlank() const
{
    return m_pCentralWidget == nullptr && m_pControlWidget == nullptr;
}

// ----------------------------------------------------------
// Public: create the top-most node if there isn't one yet
// ----------------------------------------------------------
void FSplitWidget::createRootNode()
{
    // If we already have something (leaf or splitter), do nothing
    if (!isBlank())
        return;

    createNodeInternal();
    
    // make the new root leaf active
    if (this == topMostParent())
        this->setActiveLeaf(this);
}

// ----------------------------------------------------------
// Internal: create one leaf node in this FSplitWidget
// (controls row + FViewHost)
// ----------------------------------------------------------
void FSplitWidget::createNodeInternal()
{
    // Safety: avoid double-creation
    if (!isBlank())
        return;

    // --- Top control row in its own widget -----------------------------
    m_pControlWidget = new QWidget(this);
    auto *controlLayout = new QHBoxLayout(m_pControlWidget);
    controlLayout->setContentsMargins(0, 0, 0, 0);
    controlLayout->setSpacing(0);

    auto *btnSplitH = new QPushButton(tr("Split Horizontal"), m_pControlWidget);
    auto *btnSplitV = new QPushButton(tr("Split Vertical"),   m_pControlWidget);
    auto *btnClose  = new QPushButton(tr("Close"),            m_pControlWidget);
    m_combo = new QComboBox(m_pControlWidget);

    controlLayout->addWidget(btnSplitH);
    controlLayout->addWidget(btnSplitV);
    controlLayout->addWidget(btnClose);
    controlLayout->addWidget(m_combo);
    controlLayout->addStretch();

    connect(btnSplitH, &QPushButton::clicked,
            this, &FSplitWidget::splitHorizontally);
    connect(btnSplitV, &QPushButton::clicked,
            this, &FSplitWidget::splitVertically);
    connect(btnClose,  &QPushButton::clicked,
            this, &FSplitWidget::closeNode);
    connect(m_combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, &FSplitWidget::onComboIndexChanged);

    // --- Central area: FViewHost --------------------------------------
    m_pCentralWidget = createViewWidget();

    if (auto* host = viewWidget())
    {
        connect(host, &FViewHost::activated, this, [this]() {
            // this leaf is requesting to be the active one
            this->topMostParent()->setActiveLeaf(this);
        });
    }

    m_pMainLayout->addWidget(m_pControlWidget);
    m_pMainLayout->addWidget(m_pCentralWidget);
}

FViewHost *FSplitWidget::viewWidget() const
{
    // If we are blank, or m_pCentralWidget is a splitter or child node, return nullptr
    if (!m_pCentralWidget)
        return nullptr;

    // Leaf case: central is FViewHost
    if (qobject_cast<QSplitter *>(m_pCentralWidget))
        return nullptr;

    // If central is a promoted child FSplitWidget, also not a leaf
    if (qobject_cast<FSplitWidget *>(m_pCentralWidget))
        return nullptr;

    return qobject_cast<FViewHost *>(m_pCentralWidget);
}

QWidget *FSplitWidget::createViewWidget()
{
    // Replace this with your real content widget if you want.
    auto *w = new FViewHost(this);
    w->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    return w;
}

void FSplitWidget::splitHorizontally()
{
    // 1) let outside world react first
    emit beforeSplit(this, Qt::Horizontal);

    // 2) then run the internal split logic
    makeSplit(Qt::Horizontal, nullptr);
}

void FSplitWidget::splitVertically()
{
    // 1) let outside world react first
    emit beforeSplit(this, Qt::Vertical);

    // 2) then run the internal split logic
    makeSplit(Qt::Vertical, nullptr);
}

void FSplitWidget::makeSplit(Qt::Orientation orientation, QWidget *newViewWidget)
{
    // If we are blank, create the root leaf first.
    if (isBlank())
        createNodeInternal();

    // If already a splitter, just change orientation.
    if (auto *splitter = qobject_cast<QSplitter *>(m_pCentralWidget))
    {
        splitter->setOrientation(orientation);
        return;
    }

    // Only leaf nodes can be split.
    auto *pHostView = viewWidget();
    if (!pHostView)
        return;

    bool bIsCurrentActive = pHostView->isActive();
    QWidget *pCurrentViewWidget = pHostView->takeViewWidget();

    // This node becomes an internal container: remove its controls.
    if (m_pControlWidget)
    {
        m_pMainLayout->removeWidget(m_pControlWidget);
        m_pControlWidget->hide();
        m_pControlWidget->deleteLater();
        m_pControlWidget = nullptr;
    }

    auto *splitter = new QSplitter(orientation, this);
    splitter->setHandleWidth(5);

    auto *child1 = new FSplitWidget(splitter);
    auto *child2 = new FSplitWidget(splitter);

    child1->setViewPool(m_viewPool);
    child2->setViewPool(m_viewPool);

    splitter->addWidget(child1);
    splitter->addWidget(child2);

    // Replace our leaf view host with splitter.
    m_pMainLayout->replaceWidget(m_pCentralWidget, splitter);
    m_pCentralWidget->deleteLater();
    m_pCentralWidget = splitter;

    if (bIsCurrentActive)
    {
        auto pViewChild1 = child1->viewWidget();
        if (pViewChild1)
        {
            QSignalBlocker blocker(pViewChild1);
            pViewChild1->setActive(true);
        }
    }

    // Children start blank; make them leaf nodes.
    child1->createRootNode();
    child2->createRootNode();

    // Move previous view into child1.
    if (pCurrentViewWidget)
    {
        if (auto *h1 = child1->viewWidget())
            h1->setViewWidget(pCurrentViewWidget);
    }

    // if (newViewWidget)
    // {
    //     if (auto *h2 = child2->viewWidget())
    //         h2->setViewWidget(newViewWidget);
    // }
    
    child1->updateAllViewCombos();
    child2->updateAllViewCombos();

    topMostParent()->setActiveLeaf(child2);
}

// ----------------------------------------------------------
// Helper: clear this FSplitWidget back to blank state
// ----------------------------------------------------------
void FSplitWidget::clearToBlank()
{
    if (m_pControlWidget)
    {
        m_pMainLayout->removeWidget(m_pControlWidget);
        m_pControlWidget->deleteLater();
        m_pControlWidget = nullptr;
    }

    if (m_pCentralWidget)
    {
        m_pMainLayout->removeWidget(m_pCentralWidget);
        m_pCentralWidget->deleteLater();
        m_pCentralWidget = nullptr;
    }
}

// ----------------------------------------------------------
// Close current node (leaf or internal child)
// ----------------------------------------------------------
void FSplitWidget::closeNode()
{
    // Find nearest QSplitter that directly contains some ancestor of this node.
    QWidget   *w               = this;
    QSplitter *foundSplitter   = nullptr;
    QWidget   *childInSplitter = nullptr;

    while (w)
    {
        QWidget *parent = w->parentWidget();
        if (!parent)
            break;

        if (auto *splitter = qobject_cast<QSplitter *>(parent))
        {
            foundSplitter   = splitter;
            childInSplitter = w;
            break;
        }

        w = parent;
    }

    // Case A: no splitter above -> last remaining node chain.
    // Clear the TOP-MOST FSplitWidget so the whole thing becomes truly blank again.
    if (!foundSplitter)
    {
        FSplitWidget *top = this;
        QWidget *p = this->parentWidget();
        while (auto *ps = qobject_cast<FSplitWidget *>(p))
        {
            top = ps;
            p = ps->parentWidget();
        }

        top->clearToBlank();
        return;
    }

    // Case B: normal collapse inside a 2-pane splitter.
    int idx = foundSplitter->indexOf(childInSplitter);
    if (idx == -1 || foundSplitter->count() != 2)
        return;

    int siblingIdx = (idx == 0) ? 1 : 0;

    auto *removeNode = qobject_cast<FSplitWidget *>(childInSplitter);
    auto *keepNode   = qobject_cast<FSplitWidget *>(foundSplitter->widget(siblingIdx));
    if (!removeNode || !keepNode)
        return;

    auto *container = qobject_cast<FSplitWidget *>(foundSplitter->parentWidget());
    if (!container)
        return;

    container->collapseSplitter(foundSplitter, keepNode, removeNode);
}

// ----------------------------------------------------------
// Collapse a splitter: keep one child, remove the other
// ----------------------------------------------------------
void FSplitWidget::collapseSplitter(QSplitter *splitter, FSplitWidget *keep, FSplitWidget *remove)
{
    Q_UNUSED(remove);

    // detect if active is in remove subtree
    FSplitWidget* top = topMostParent();
    FSplitWidget* curActive = top->activeNode();

    bool activeWasRemoved = false;
    if (curActive)
        activeWasRemoved = remove->leafNodes().contains(curActive);

    if (splitter != m_pCentralWidget)
        return;

    // Promote the surviving node.
    keep->setParent(this);
    m_pMainLayout->replaceWidget(splitter, keep);
    m_pCentralWidget = keep;

    splitter->deleteLater();

    // Optional pruning: if the promoted node is blank, don't keep an empty branch around.
    if (auto *promoted = qobject_cast<FSplitWidget *>(m_pCentralWidget))
    {
        if (promoted->isBlank())
        {
            m_pMainLayout->removeWidget(promoted);
            promoted->deleteLater();
            m_pCentralWidget = nullptr;
        }
    }

    if (activeWasRemoved)
    {
        auto keepLeaves = keep->leafNodes();
        if (!keepLeaves.isEmpty())
            top->setActiveLeaf(keepLeaves.first());
    }
}

FSplitWidget* FSplitWidget::activeNode()
{
    auto* top = topMostParent();
    const auto leaves = top->leafNodes();

    for (auto* n : leaves)
    {
        if (auto* host = n->viewWidget())
        {
            if (host->isActive())
                return n;
        }
    }
    return nullptr;
}

void FSplitWidget::setActiveLeaf(FSplitWidget* leaf)
{
    if (!leaf)
        return;

    // Always operate on the top-most parent
    auto* top = topMostParent();
    if (top != this)
    {
        top->setActiveLeaf(leaf);
        return;
    }

    const auto leaves = leafNodes();
    if (!leaves.contains(leaf))
        return;

    // // If already active, do nothing
    // if (auto* cur = activeNode())
    // {
    //     if (cur == leaf)
    //         return;
    // }

    bool emitSignal = false;
    // Turn all inactive, only 'leaf' active
    for (auto* n : leaves)
    {
        if (auto* host = n->viewWidget())
        {
            const bool shouldBeActive = (n == leaf);

            if (host->isActive() != shouldBeActive)
            {
                host->setActive(shouldBeActive);
                emitSignal = true;
            }
            
            // if (shouldBeActive)
            // {
            //     // block signals so setActive(true) doesn't re-trigger activated() handling
            //     // QSignalBlocker blocker(host);
            //     host->setActive(shouldBeActive);
            // }
            // else
            //     host->setActive(shouldBeActive);
        }
    }

    if (emitSignal)
        emit activeNodeChanged(leaf);
}

void FSplitWidget::setViewPool(std::unordered_map<QWidget*, QString>* pool)
{
    m_viewPool = pool;
}

QWidget* FSplitWidget::currentLeafView() const
{
    auto* host = viewWidget();  // your leaf host accessor
    if (!host) return nullptr;
    return host->currentViewWidget();  // current view inside FViewHost
}

void FSplitWidget::updateComboForLeaf(const QList<QWidget*>& unusedWidgets)
{
    if (!m_combo || !m_viewPool) return;

    QWidget* cur = currentLeafView();

    QSignalBlocker b(m_combo);
    m_combo->clear();

    // Optional <None>
    m_combo->addItem("<None>", QVariant::fromValue((QWidget*)nullptr));

    auto addWidgetItem = [&](QWidget* w)
    {
        if (!w) return;
        auto it = m_viewPool->find(w);
        QString name = (it != m_viewPool->end()) ? it->second : QString("<unnamed>");
        m_combo->addItem(name, QVariant::fromValue(w));
    };

    // 1) Add current first (if any)
    if (cur) addWidgetItem(cur);

    // 2) Add unused (skip if equals current)
    for (QWidget* w : unusedWidgets)
        if (w != cur)
            addWidgetItem(w);

    // Select current item
    if (!cur)
    {
        m_combo->setCurrentIndex(0); // <None>
    }
    else
    {
        // find item by pointer stored in data
        for (int i = 0; i < m_combo->count(); ++i)
        {
            QWidget* itemW = m_combo->itemData(i).value<QWidget*>();
            if (itemW == cur)
            {
                m_combo->setCurrentIndex(i);
                break;
            }
        }
    }
}

void FSplitWidget::updateAllViewCombos()
{
    FSplitWidget* top = topMostParent();
    if (top != this) { top->updateAllViewCombos(); return; }

    if (!m_viewPool) return;

    const auto leaves = leafNodes();

    // Collect used QWidget* pointers
    QSet<QWidget*> used;
    for (auto* leaf : leaves)
    {
        if (QWidget* w = leaf->currentLeafView())
            used.insert(w);
    }

    // Build list of unused widgets from the pool
    QList<QWidget*> unused;
    unused.reserve((int)m_viewPool->size());
    for (const auto& kv : *m_viewPool)
    {
        QWidget* w = kv.first;
        if (w && !used.contains(w))
            unused.push_back(w);
    }

    // Update each leaf combo: (its current widget) + (unused widgets)
    for (auto* leaf : leaves)
        leaf->updateComboForLeaf(unused);
}

void FSplitWidget::onComboIndexChanged(int /*index*/)
{
    if (!m_viewPool) return;

    auto* host = viewWidget();
    if (!host) return; // only leaf nodes have a combo

    QWidget* selected = m_combo->currentData().value<QWidget*>();
    host->setViewWidget(selected);

    topMostParent()->updateAllViewCombos();
}