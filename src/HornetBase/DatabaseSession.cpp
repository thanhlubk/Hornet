#include "HornetBase/DatabaseSession.h"
#include "HornetBase/HItemManager.h"
#include "HornetBase/PoolMix.h"
#include "HornetBase/PoolUnique.h"
#include <stdexcept>

DatabaseSession::DatabaseSession(std::size_t chunk_bytes_per_store, bool lazy_first_chunk)
    : m_iChunkBytes(chunk_bytes_per_store),
      m_bLazy(lazy_first_chunk)
{
    // default policy for each CategoryType
    m_mapCategoryPoolType[CategoryType::CatNode] = PoolType::Unique;
    m_mapCategoryPoolType[CategoryType::CatElement] = PoolType::Mix;
    m_mapCategoryPoolType[CategoryType::CatGroup] = PoolType::Unique;
}

void DatabaseSession::add(CategoryType cat)
{
    PoolType pt = static_cast<PoolType>(m_mapCategoryPoolType.at(cat));
    auto &pPool = m_mapCategoryPool[cat];
    if (!pPool)
    {
        if (pt == PoolType::Unique)
            pPool = std::make_unique<PoolUnique>(cat, m_iChunkBytes, m_bLazy);
        else
            pPool = std::make_unique<PoolMix>(cat, m_iChunkBytes, m_bLazy);
    }
}

Pool *DatabaseSession::ensure(CategoryType cat)
{
    auto it = m_mapCategoryPool.find(cat);
    if (it == m_mapCategoryPool.end())
    {
        add(cat);
        it = m_mapCategoryPool.find(cat);
    }
    return it->second.get();
}

bool DatabaseSession::contain(CategoryType cat) const
{
    return m_mapCategoryPool.find(cat) != m_mapCategoryPool.end();
}

Pool *DatabaseSession::getPool(CategoryType cat)
{
    auto it = m_mapCategoryPool.find(cat);
    return (it == m_mapCategoryPool.end()) ? nullptr : it->second.get();
}

const Pool *DatabaseSession::getPool(CategoryType cat) const
{
    auto it = m_mapCategoryPool.find(cat);
    return (it == m_mapCategoryPool.end()) ? nullptr : it->second.get();
}

PoolUnique *DatabaseSession::getPoolUnique(CategoryType cat)
{
    if (auto *p = getPool(cat))
        return dynamic_cast<PoolUnique *>(p);
    return nullptr;
}

PoolMix *DatabaseSession::getPoolMix(CategoryType cat)
{
    if (auto *p = getPool(cat))
        return dynamic_cast<PoolMix *>(p);
    return nullptr;
}

void DatabaseSession::clearCategory(CategoryType cat)
{
    auto it = m_mapCategoryPool.find(cat);
    if (it == m_mapCategoryPool.end())
        return;
    it->second->clear();
    m_mapCategoryPool.erase(it);
}

void DatabaseSession::clear()
{
    for (auto &kv : m_mapCategoryPool)
        kv.second->clear();
    m_mapCategoryPool.clear();
}

DatabaseSession::Stats DatabaseSession::stats() const
{
    Stats s{};
    s.store_count = m_mapCategoryPool.size();
    for (auto &kv : m_mapCategoryPool)
    {
        s.bytes_used += kv.second->bytesUsed();
        s.objects += kv.second->count();
    }
    return s;
}

bool DatabaseSession::isTransactionOpen() const noexcept
{
    return m_transaction.isOpen();
}

void DatabaseSession::beginTransaction()
{
    m_transaction.begin();
}

void DatabaseSession::commitTransaction()
{
    if (!m_transaction.isOpen())
        return;

    auto &cur = m_transaction.get_current_transaction();

    // Capture AFTER bytes for Modify/Emplace
    for (auto &[k, op] : cur)
    {
        if (op.type == TransactionManager::TransactionType::Modify ||
            op.type == TransactionManager::TransactionType::Emplace)
        {
            if (auto *store = getPool(HItemManager::getInstance().getCategory(op.itemType)))
            {
                if (auto const *p = store->getRawConst(op.itemType, op.id))
                {
                    op.payloadAfter = HItemManager::getInstance().captureTransaction(op.itemType, p);
                }
            }
        }
    }

    // Apply commit direction
    for (auto &[k, op] : cur)
    {
        if (auto *store = getPool(HItemManager::getInstance().getCategory(op.itemType)))
            store->updateBytes(op);
    }

    m_transaction.commit();
}

void DatabaseSession::rollbackTransaction()
{
    if (!m_transaction.isOpen())
        return;

    auto &cur = m_transaction.get_current_transaction();
    for (auto it = cur.begin(); it != cur.end(); ++it)
    {
        if (auto *store = getPool(HItemManager::getInstance().getCategory(it->second.itemType)))
            store->restoreBytes(it->second);
    }
    m_transaction.rollback();
}

bool DatabaseSession::undo()
{
    if (m_transaction.isOpen())
        return false;

    const TransactionManager::Transaction *tx = nullptr;
    if (m_transaction.undo(tx) && tx)
    {
        for (auto it = tx->ops.rbegin(); it != tx->ops.rend(); ++it)
        {
            const auto &op = *it;
            auto *store = getPool(HItemManager::getInstance().getCategory(op.itemType));
            if (!store)
                continue;

            switch (op.type)
            {
            case TransactionManager::TransactionType::Emplace:
                // Undo insert → erase it
                store->restoreBytes(op);
                break;
            case TransactionManager::TransactionType::Erase:
                // Undo erase → recreate and apply BEFORE
                (void)undoUpdateBytes(op);
                break;
            case TransactionManager::TransactionType::Modify:
                // Restore BEFORE
                store->restoreBytes(op);
                break;
            }
        }
        return true;
    }
    return false;
}

bool DatabaseSession::redo()
{
    if (m_transaction.isOpen())
        return false;

    const TransactionManager::Transaction *tx = nullptr;
    if (m_transaction.redo(tx) && tx)
    {
        for (auto &op : tx->ops)
        {
            auto *store = getPool(HItemManager::getInstance().getCategory(op.itemType));
            if (!store)
                continue;

            switch (op.type)
            {
            case TransactionManager::TransactionType::Emplace:
                // Redo insert → recreate and apply AFTER
                (void)redoUpdateBytes(op);
                break;
            case TransactionManager::TransactionType::Erase:
                // Redo erase → commit direction
                store->updateBytes(op);
                break;
            case TransactionManager::TransactionType::Modify:
                // Apply AFTER
                store->updateBytes(op);
                break;
            }
        }
        return true;
    }
    return false;
}

// Recreate an erased object and restore its BEFORE state (used by UNDO of Erase)
bool DatabaseSession::undoUpdateBytes(const TransactionManager::TransactionOperation &op)
{
    CategoryType cat = HItemManager::getInstance().getCategory(op.itemType);
    auto *store = getPool(cat);
    if (!store)
        return false;

    HItemCreatorToken tok{};
    if (!store->emplaceRaw(op.itemType, op.id, tok))
        return false;

    if (void *p = store->getRaw(op.itemType, op.id))
    {
        HItemManager::getInstance().restoreTransaction(op.itemType, p, op.payloadBefore);
        return true;
    }
    return false;
}

// Recreate an inserted object and restore its AFTER state (used by REDO of Emplace)
bool DatabaseSession::redoUpdateBytes(const TransactionManager::TransactionOperation &op)
{
    CategoryType cat = HItemManager::getInstance().getCategory(op.itemType);
    auto *store = getPool(cat);
    if (!store)
        return false;

    if (void *p = store->getRaw(op.itemType, op.id))
    {
        // already exists: idempotent
        HItemManager::getInstance().restoreTransaction(op.itemType, p, op.payloadAfter);
        return true;
    }

    HItemCreatorToken tok{};
    if (!store->emplaceRaw(op.itemType, op.id, tok))
        return false;

    if (void *p2 = store->getRaw(op.itemType, op.id))
    {
        HItemManager::getInstance().restoreTransaction(op.itemType, p2, op.payloadAfter);
        return true;
    }
    return false;
}
