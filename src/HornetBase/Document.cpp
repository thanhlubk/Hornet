#include "HornetBase/Document.h"

Document::Document(DocumentType type)
    : m_dispatcher(std::make_unique<NotifyDispatcher>())
    , m_type(type)
{
}

void Document::notifySomething()
{
    // Example: you can implement a generic notification here later.
    // if (auto* disp = dispatcher()) {
    //     disp->notify(MessageType::Something,
    //                  NotifyDispatcher::toWord(this),
    //                  0);
    // }
}
