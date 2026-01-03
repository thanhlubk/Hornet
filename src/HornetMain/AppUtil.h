// HornetMain/AppAccess.h
#pragma once

#include <QCoreApplication>
#include "HornetApp.h"

inline HornetApp* mainApp() noexcept
{
    return qobject_cast<HornetApp*>(QCoreApplication::instance());
}

inline AppBase* appBase() noexcept
{
    auto* ha = mainApp();
    return ha ? ha->appBase() : nullptr;
}

// convenience helpers if you like:
inline DocumentManager* docs() noexcept
{
    auto* a = appBase();
    return a ? a->docs() : nullptr;
}

inline Setting* setting() noexcept
{
    auto* a = appBase();
    return a ? a->setting() : nullptr;
}
