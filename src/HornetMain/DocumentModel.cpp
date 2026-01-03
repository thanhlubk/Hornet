// DocumentModel.cpp
#include "DocumentModel.h"
#include <HornetView/GLViewWindow.h>
#include <QWidget>

DocumentModel::DocumentModel()
    : Document(DocumentType::Model)
    , m_db(std::make_unique<DatabaseSession>())
{
}

DocumentModel::~DocumentModel()
{
// Case 1: Container exists -> it owns the GLViewWindow (QWindow)
    // Deleting the container deletes the embedded window too.
    if (m_pViewWidget) {
        delete m_pViewWidget;
        m_pViewWidget = nullptr;

        // window was owned by container; pointer is no longer valid
        m_pView = nullptr;
    }

    // Case 2: No container, but window exists (should be rare).
    // This can happen if you created the window but never wrapped it,
    // or you detached it from the container earlier.
    if (m_pView) {
        delete m_pView;
        m_pView = nullptr;
    }
}

void DocumentModel::setView(GLViewWindow* view)
{
    if (!view)
        return;
    
    m_pView = view; 
    m_pView->setNotifyDispatcher(*dispatcher());

    m_pViewWidget = QWidget::createWindowContainer(m_pView, nullptr);// get QWidget wrapper
    // If you want strong focus / mouse tracking, set it on the container:
    m_pViewWidget->setFocusPolicy(Qt::StrongFocus);
    m_pViewWidget->setMouseTracking(true);
    m_pViewWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
}

bool DocumentModel::loadFromFile(const std::string& path)
{
    // TODO: implement real loading logic using *m_db
    // if (auto* db = database()) { ... }

    m_filePath = path;
    setDirty(false);
    return true;
}

bool DocumentModel::saveToFile(const std::string& path)
{
    std::string target = path.empty() ? m_filePath : path;
    if (target.empty())
        return false;

    // TODO: implement real saving logic using *m_db
    // if (auto* db = database()) { ... }

    m_filePath = target;
    setDirty(false);
    return true;
}

void DocumentModel::setDirty(bool dirty)
{
    if (m_dirty == dirty)
        return;

    m_dirty = dirty;

    // Optional: notify via dispatcher
    // if (auto* disp = dispatcher()) {
    //     disp->notify(MessageType::DataModified,
    //                  NotifyDispatcher::toWord(this),
    //                  dirty ? 1 : 0);
    // }
}
