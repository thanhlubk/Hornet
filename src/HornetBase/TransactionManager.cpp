#include <stdexcept>
#include <utility>
#include "HornetBase/TransactionManager.h"

TransactionManager::TransactionManager()
{
    m_vecUndoStack.clear();
    m_vecRedoStack.clear();
    m_mapCurrentTransaction.clear();
    m_bIsOpen = false;
}

bool TransactionManager::isOpen() const noexcept
{
    return m_bIsOpen;
}

void TransactionManager::begin()
{
    if (m_bIsOpen)
        throw std::logic_error("Transaction already open");
    m_bIsOpen = true;
}

void TransactionManager::rollback()
{
    require_tx();
    m_mapCurrentTransaction.clear();
    m_bIsOpen = false;
}

void TransactionManager::commit()
{
    require_tx();

    Transaction tx;
    tx.ops.reserve(m_mapCurrentTransaction.size());
    for (auto &[k, op] : m_mapCurrentTransaction)
        tx.ops.push_back(op);

    m_vecUndoStack.push_back(std::move(tx));
    m_mapCurrentTransaction.clear();
    m_vecRedoStack.clear();
    m_bIsOpen = false;
}

bool TransactionManager::undo(const Transaction *&current_tx)
{
    if (m_bIsOpen)
        throw std::logic_error("Close transaction before undo");
    if (m_vecUndoStack.empty())
        return false;

    Transaction tx = std::move(m_vecUndoStack.back());
    m_vecUndoStack.pop_back();
    m_vecRedoStack.push_back(std::move(tx));
    current_tx = &m_vecRedoStack.back();
    return true;
}

bool TransactionManager::redo(const Transaction *&current_tx)
{
    if (m_bIsOpen)
        throw std::logic_error("Close transaction before redo");
    if (m_vecRedoStack.empty())
        return false;

    Transaction tx = std::move(m_vecRedoStack.back());
    m_vecRedoStack.pop_back();
    m_vecUndoStack.push_back(std::move(tx));
    current_tx = &m_vecUndoStack.back();
    return true;
}

std::unordered_map<Key, TransactionManager::TransactionOperation, KeyHash> &
TransactionManager::get_current_transaction()
{
    return m_mapCurrentTransaction;
}

void TransactionManager::require_tx() const
{
    if (!m_bIsOpen)
        throw std::logic_error("Writes allowed only in an open transaction");
}
