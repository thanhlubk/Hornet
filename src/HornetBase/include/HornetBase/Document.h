#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include "HornetBaseExport.h"
#include "NotifyDispatcher.h"

enum class DocumentType : std::uint8_t
{
    Unknown = 0,
    Model,
    Chart,
    Compare,
};

class HORNETBASE_EXPORT Document
{
public:
    explicit Document(DocumentType type = DocumentType::Unknown);
    virtual ~Document() = default;

    // Type
    DocumentType type() const noexcept { return m_type; }
    void setType(DocumentType type) noexcept { m_type = type; }

    // Per-document dispatcher (may be null in specialized docs if you choose)
    NotifyDispatcher* dispatcher() noexcept { return m_dispatcher.get(); }
    // const NotifyDispatcher* dispatcher() const noexcept { return m_dispatcher.get(); }

    // Metadata
    const std::string& name() const noexcept { return m_strName; }
    void setName(std::string name) { m_strName = std::move(name); }

    void notify(MessageType type, MessageParam wparam = 0, MessageParam lparam = 0)
    {
        if (auto* disp = dispatcher())
        {
            disp->notify(type, wparam, lparam);
        }
    }
protected:
    void notifySomething(); // placeholder hook if you want a generic event

private:
    std::unique_ptr<NotifyDispatcher> m_dispatcher;
    std::string                 m_strName;
    DocumentType                m_type = DocumentType::Unknown;
};
