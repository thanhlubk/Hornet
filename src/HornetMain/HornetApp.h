// HornetMain/HornetApp.h
#pragma once
#include <QApplication>
#include <HornetBase/AppBase.h>

class HornetApp : public QApplication
{
public:
    HornetApp(int& argc, char** argv)
        : QApplication(argc, argv)
        , m_appBase(std::make_unique<AppBase>())
    {}

    ~HornetApp() override = default;

    // Pointer access (can return nullptr if you ever reset it)
    AppBase* appBase() noexcept                { return m_appBase.get(); }
    const AppBase* appBase() const noexcept    { return m_appBase.get(); }

    // If you ever need to replace the core (e.g. tests, reload)
    void resetAppBase(std::unique_ptr<AppBase> appBase)
    {
        m_appBase = std::move(appBase);
    }

private:
    std::unique_ptr<AppBase> m_appBase;  // single AppBase instance
};