// DocumentChart.cpp
#include "DocumentChart.h"

#include <QWidget>  // only needed in the .cpp; header just forward-declares QWidget

DocumentChart::DocumentChart()
    : Document(DocumentType::Chart)
    , m_db(std::make_unique<DatabaseSession>())
{
}

void DocumentChart::setView(QWidget* view)
{
    m_view = view;
    // No QObject inheritance on DocumentChart, so don’t connect signals here.
    // If the view needs to listen to dispatcher() or other stuff, it can:
    //   - get the DocumentChart* from outside, or
    //   - attach directly to dispatcher() from wherever you create both.
}

bool DocumentChart::loadFromFile(const std::string& path)
{
    // TODO: implement actual load logic.
    // Example if you want to load via DB:
    //
    // if (auto* db = database()) {
    //     if (!db->loadChart(path)) {
    //         return false;
    //     }
    // }

    m_filePath = path;
    setDirty(false);
    return true;
}

bool DocumentChart::saveToFile(const std::string& path)
{
    std::string target = path.empty() ? m_filePath : path;
    if (target.empty())
        return false;

    // TODO: implement actual save logic.
    // Example:
    //
    // if (auto* db = database()) {
    //     if (!db->saveChart(target)) {
    //         return false;
    //     }
    // }

    m_filePath = target;
    setDirty(false);
    return true;
}

void DocumentChart::setDirty(bool dirty)
{
    if (m_dirty == dirty)
        return;

    m_dirty = dirty;

    // Optional: notify chart listeners via the per-document dispatcher.
    // if (auto* disp = dispatcher()) {
    //     disp->notify(MessageType::DataModified,
    //                  NotifyDispatcher::toWord(this),
    //                  dirty ? 1 : 0);
    // }
}
