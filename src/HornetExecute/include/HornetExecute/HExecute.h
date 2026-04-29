#pragma once
#include "HornetExecuteExport.h"
#include <HornetBase/DatabaseSession.h>

class HORNETEXECUTE_EXPORT HExecute
{
public:
    HExecute(DatabaseSession* db, bool transaction=true, bool logCommand=true);
    virtual ~HExecute() = default;
    bool execute();

protected:
    virtual bool onExecute() = 0;
    virtual void logCommand() = 0;

    DatabaseSession* m_pDb;
    bool m_bTransaction;
    bool m_bLogCommand;
};