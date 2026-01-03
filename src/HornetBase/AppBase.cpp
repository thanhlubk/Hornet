// AppBase.cpp
#include "HornetBase/AppBase.h"
#include "HornetBase/DocumentManager.h"
#include "HornetBase/Setting.h"

AppBase::AppBase()
    : m_docs(std::make_unique<DocumentManager>())
    , m_setting(std::make_unique<Setting>())
{
}

AppBase::~AppBase() = default;
