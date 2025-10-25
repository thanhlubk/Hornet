#pragma once
#include <cstdint>
#include <functional>
#include <vector>
#include <mutex>
#include <algorithm>
#include <atomic>
#include <memory>
#include "HornetBaseExport.h"

// ---------- Messages ----------
enum class MessageType : std::uint16_t
{
    DataEmplaced = 1,
    DataErased = 2,
    DataModified = 3,
    CategoryCleared = 10,
    AllCleared = 11,

    TxBegin = 100,
    TxCommit = 101,
    TxRollback = 102,
    TxUndoApplied = 103,
    TxRedoApplied = 104,

    SelectionChanged = 200,
    ViewRequestRedraw = 201,

    UserBase = 1000
};

// Use a portable pointer-sized word everywhere
using MessageParam = std::uintptr_t;

// Optional helpers for payloads
template <class T>
inline MessageParam toWord(T *p) noexcept { return reinterpret_cast<MessageParam>(p); }
template <class T>
inline T *fromWordPtr(MessageParam w) noexcept { return reinterpret_cast<T *>(w); }

inline MessageParam toWordU64(std::uint64_t v) noexcept { return static_cast<MessageParam>(v); }
inline MessageParam toWordI64(std::int64_t v) noexcept { return static_cast<MessageParam>(v); }
inline std::uint64_t fromWordU64(MessageParam w) noexcept { return static_cast<std::uint64_t>(w); }
inline std::int64_t fromWordI64(MessageParam w) noexcept { return static_cast<std::int64_t>(w); }

// --------------------------------------------------------------
// Per-document dispatcher with Core + weak_ptr (Option B).
// Store only an Observer in your components.
// --------------------------------------------------------------
class HORNETBASE_EXPORT NotifyDispatcher
{
public:
    using SubId = std::uint64_t;
    using Callback = std::function<void(MessageType, MessageParam, MessageParam)>;

    NotifyDispatcher();
    ~NotifyDispatcher() = default;
    NotifyDispatcher(const NotifyDispatcher &) = delete;
    NotifyDispatcher &operator=(const NotifyDispatcher &) = delete;
    NotifyDispatcher(NotifyDispatcher &&) noexcept = default;
    NotifyDispatcher &operator=(NotifyDispatcher &&) noexcept = default;

    // Forward declaration of the internal Core so Observer can refer to it
private:
    struct Core;

public:
    // ---------------- Observer (store this) ----------------
    class HORNETBASE_EXPORT Observer
    {
    public:
        Observer() = default;
        Observer(const Observer &) = delete;
        Observer &operator=(const Observer &) = delete;

        Observer(Observer &&obs) noexcept { *this = std::move(obs); }
        Observer &operator=(Observer &&obs) noexcept
        {
            if (this != &obs)
            {
                reset();
                m_pCore = std::move(obs.m_pCore);
                m_iId = obs.m_iId;
                obs.m_iId = 0;
            }
            return *this;
        }

        ~Observer() { reset(); }

        void reset();
        bool isSubscribed() const noexcept { return m_iId != 0 && !m_pCore.expired(); }
        SubId id() const noexcept { return m_iId; }

        // Send
        void notify(MessageType type, MessageParam wparam = 0, MessageParam lparam = 0) const;
        void notify(MessageType type, MessageParam wparam, MessageParam lparam, bool includeSelf) const;
        void notifyOthers(MessageType type, MessageParam wparam = 0, MessageParam lparam = 0) const;

    private:
        friend class NotifyDispatcher;
        void attach(const std::shared_ptr<NotifyDispatcher::Core> &core, SubId id)
        {
            reset();
            m_pCore = core;
            m_iId = id;
        }

        std::weak_ptr<NotifyDispatcher::Core> m_pCore;
        SubId m_iId = 0;
    };

    // ---------------- Attach APIs ----------------
    Observer attach(Callback callbackFunc);                       // all messages
    Observer attachOnly(MessageType type, Callback callbackFunc); // one type

    template <class T>
    Observer attach(T *obj, void (T::*method)(MessageType, MessageParam, MessageParam))
    {
        return attach([obj, method](MessageType mt, MessageParam a, MessageParam b)
                      { (obj->*method)(mt, a, b); });
    }
    template <class T>
    Observer attachOnly(MessageType type, T *obj, void (T::*method)(MessageType, MessageParam, MessageParam))
    {
        return attachOnly(type, [obj, method](MessageType mt, MessageParam a, MessageParam b)
                          { (obj->*method)(mt, a, b); });
    }

    // ---------------- Broadcast ----------------
    void notify(MessageType type, MessageParam wparam = 0, MessageParam lparam = 0);
    void notifyExclude(SubId exclude, MessageType type, MessageParam wparam = 0, MessageParam lparam = 0);

private:
    // --------------- Core (shared state) ---------------
    struct Core
    {
        struct Slot
        {
            SubId id;
            MessageType filter; // kAllSentinel => "all"
            bool all;
            Callback callbackFunc;
        };

        std::mutex mx;
        std::vector<Slot> vecSlot;
        std::atomic<SubId> nextId{0};

        static constexpr MessageType kAllSentinel = static_cast<MessageType>(0);

        SubId attach(Callback callbackFunc);
        SubId attachOnly(MessageType type, Callback callbackFunc);
        void detach(SubId id);

        void notify(MessageType type, MessageParam wparam, MessageParam lparam);
        void notifyExclude(SubId exclude, MessageType type, MessageParam wparam, MessageParam lparam);
    };

    std::shared_ptr<Core> m_pCore; // owner
};
