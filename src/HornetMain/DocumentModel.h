// DocumentModel.h
#pragma once

#include <memory>
#include <string>
#include <HornetBase/Document.h>
#include <HornetBase/DatabaseSession.h>
#include <HornetBase/DatabaseSession.h>

class GLViewWindow;
class QWidget;

class DocumentModel : public Document
{
public:
    DocumentModel();
    ~DocumentModel() override;

    // Access per-DocumentModel DB and dispatcher
    DatabaseSession*       database()       noexcept { return m_db.get(); }
    const DatabaseSession* database() const noexcept { return m_db.get(); }
    
    // View attachment (optional; nullptr if headless)
    GLViewWindow* view() const noexcept { return m_pView; }
    QWidget* viewWidget() const noexcept { return m_pViewWidget; }
    void setView(GLViewWindow* view);

    // File / persistence
    const std::string& filePath() const noexcept { return m_filePath; }
    void setFilePath(std::string path) { m_filePath = std::move(path); }

    bool isDirty() const noexcept { return m_dirty; }

    virtual bool loadFromFile(const std::string& path);
    virtual bool saveToFile(const std::string& path = std::string());

protected:
    virtual void setDirty(bool dirty);

private:
    std::unique_ptr<DatabaseSession> m_db;   // can be nullptr in other doc types if you want
    GLViewWindow*                    m_pView = nullptr;  // not owned
    QWidget*                         m_pViewWidget = nullptr;  // not owned

    std::string                      m_filePath;
    bool                             m_dirty = false;
};
