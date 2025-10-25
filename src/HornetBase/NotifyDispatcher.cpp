#include "HornetBase/NotifyDispatcher.h"

// ---------- NotifyDispatcher ----------
NotifyDispatcher::NotifyDispatcher()
    : m_pCore(std::make_shared<Core>()) {}

// Attach (all)
NotifyDispatcher::Observer NotifyDispatcher::attach(Callback callbackFunc)
{
    Observer obs;
    if (m_pCore)
    {
        const SubId id = m_pCore->attach(std::move(callbackFunc));
        obs.attach(m_pCore, id);
    }
    return obs;
}

// Attach (one)
NotifyDispatcher::Observer NotifyDispatcher::attachOnly(MessageType type, Callback callbackFunc)
{
    Observer obs;
    if (m_pCore)
    {
        const SubId id = m_pCore->attachOnly(type, std::move(callbackFunc));
        obs.attach(m_pCore, id);
    }
    return obs;
}

// Broadcast
void NotifyDispatcher::notify(MessageType type, MessageParam wparam, MessageParam lparam)
{
    if (m_pCore)
        m_pCore->notify(type, wparam, lparam);
}
void NotifyDispatcher::notifyExclude(SubId exclude, MessageType type, MessageParam wparam, MessageParam lparam)
{
    if (m_pCore)
        m_pCore->notifyExclude(exclude, type, wparam, lparam);
}

// ---------- Observer ----------
void NotifyDispatcher::Observer::reset()
{
    if (m_iId == 0)
        return;
    if (auto core = m_pCore.lock())
    {
        core->detach(m_iId);
    }
    m_iId = 0;
}

void NotifyDispatcher::Observer::notify(MessageType type, MessageParam wparam, MessageParam lparam) const
{
    if (auto core = m_pCore.lock())
    {
        core->notify(type, wparam, lparam);
    }
}

void NotifyDispatcher::Observer::notify(MessageType type, MessageParam wparam, MessageParam lparam, bool includeSelf) const
{
    if (auto core = m_pCore.lock())
    {
        if (includeSelf || m_iId == 0)
            core->notify(type, wparam, lparam);
        else
            core->notifyExclude(m_iId, type, wparam, lparam);
    }
}

void NotifyDispatcher::Observer::notifyOthers(MessageType type, MessageParam wparam, MessageParam lparam) const
{
    if (auto core = m_pCore.lock())
    {
        if (m_iId == 0)
            core->notify(type, wparam, lparam);
        else
            core->notifyExclude(m_iId, type, wparam, lparam);
    }
}

// ---------- Core ----------
NotifyDispatcher::SubId
NotifyDispatcher::Core::attach(Callback callbackFunc)
{
    std::lock_guard<std::mutex> lk(mx);
    const SubId id = ++nextId;
    vecSlot.push_back(Slot{id, kAllSentinel, true, std::move(callbackFunc)});
    return id;
}

NotifyDispatcher::SubId
NotifyDispatcher::Core::attachOnly(MessageType type, Callback callbackFunc)
{
    std::lock_guard<std::mutex> lk(mx);
    const SubId id = ++nextId;
    vecSlot.push_back(Slot{id, type, false, std::move(callbackFunc)});
    return id;
}

void NotifyDispatcher::Core::detach(SubId id)
{
    if (!id)
        return;
    std::lock_guard<std::mutex> lk(mx);
    vecSlot.erase(std::remove_if(vecSlot.begin(), vecSlot.end(),
                               [&](const Slot &s)
                               { return s.id == id; }),
                vecSlot.end());
}

void NotifyDispatcher::Core::notify(MessageType type, MessageParam wparam, MessageParam lparam)
{
    std::vector<Slot> copy;
    {
        std::lock_guard<std::mutex> lk(mx);
        copy = vecSlot;
    }
    for (auto &s : copy)
    {
        if (s.all || s.filter == type)
        {
            s.callbackFunc(type, wparam, lparam);
        }
    }
}

void NotifyDispatcher::Core::notifyExclude(SubId exclude, MessageType type, MessageParam wparam, MessageParam lparam)
{
    std::vector<Slot> copy;
    {
        std::lock_guard<std::mutex> lk(mx);
        copy = vecSlot;
    }
    for (auto &s : copy)
    {
        if (s.id == exclude)
            continue;
        if (s.all || s.filter == type)
        {
            s.callbackFunc(type, wparam, lparam);
        }
    }
}
