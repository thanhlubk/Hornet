#pragma once
#include "HornetExecute/HExecute.h"

HExecute::HExecute(DatabaseSession* db, bool transaction, bool logCommand)
    : m_pDb(db), m_bTransaction(transaction), m_bLogCommand(logCommand)
{
}

bool HExecute::execute()
{
    if (!m_pDb)
        return false;
    
    if (m_bTransaction)
    {
        m_pDb->beginTransaction();
        if (!onExecute())
        {
            m_pDb->rollbackTransaction();
            return false;
        }
        if (m_bLogCommand)
            logCommand();
        
        m_pDb->commitTransaction();
    }
    
    return true;
}