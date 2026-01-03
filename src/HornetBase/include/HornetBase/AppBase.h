// AppBase.h
#pragma once
#include <memory>
#include "HornetBaseExport.h"

class DocumentManager;
class Setting;

class HORNETBASE_EXPORT AppBase
{
public:
    AppBase();
    ~AppBase();

    AppBase(const AppBase&) = delete;
    AppBase& operator=(const AppBase&) = delete;

    DocumentManager* docs() noexcept       { return m_docs.get(); }
    const DocumentManager* docs() const noexcept { return m_docs.get(); }

    Setting*       setting()       noexcept { return m_setting.get(); }
    const Setting* setting() const noexcept { return m_setting.get(); }

private:
    std::unique_ptr<DocumentManager> m_docs;
    std::unique_ptr<Setting>       m_setting;
};
