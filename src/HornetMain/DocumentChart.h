// DocumentChart.h
#pragma once

#include <memory>
#include <string>

#include <HornetBase/Document.h>
#include <HornetBase/DatabaseSession.h>

class QWidget;

// A chart-type document: owns its own DatabaseSession (optional usage),
// and has an associated QWidget-based view (e.g. some chart widget).
class DocumentChart : public Document
{
public:
    DocumentChart();
    ~DocumentChart() override = default;

    // Access per-DocumentChart DatabaseSession
    // Can be nullptr if you later decide some charts don't use a DB.
    DatabaseSession*       database()       noexcept { return m_db.get(); }
    const DatabaseSession* database() const noexcept { return m_db.get(); }

    // View attachment (optional; nullptr in headless / no-GUI mode)
    QWidget* view() const noexcept { return m_view; }
    void setView(QWidget* view);

    // File / persistence metadata
    const std::string& filePath() const noexcept { return m_filePath; }
    void setFilePath(std::string path) { m_filePath = std::move(path); }

    bool isDirty() const noexcept { return m_dirty; }

    // Load/save chart data (via DatabaseSession or directly)
    virtual bool loadFromFile(const std::string& path);
    virtual bool saveToFile(const std::string& path = std::string());

protected:
    virtual void setDirty(bool dirty);

private:
    std::unique_ptr<DatabaseSession> m_db;   // per-chart DB; adjust if not needed
    QWidget*                         m_view = nullptr; // not owned
    std::string                      m_filePath;
    bool                             m_dirty = false;
};
