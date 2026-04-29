#include "HornetBase/DatabaseSession.h"
#include "HornetBase/HItemManager.h"
#include "HornetBase/PoolMix.h"
#include "HornetBase/PoolUnique.h"
#include <stdexcept>

DatabaseSession::DatabaseSession(std::size_t chunk_bytes_per_store, bool lazy_first_chunk)
    : m_iChunkBytes(chunk_bytes_per_store),
      m_bLazy(lazy_first_chunk), m_chunkCursor(m_iChunkBytes, m_bLazy)
{
    // default policy for each CategoryType
    m_mapCategoryPoolType[CategoryType::CatNode] = PoolType::Unique;
    m_mapCategoryPoolType[CategoryType::CatElement] = PoolType::Mix;
    m_mapCategoryPoolType[CategoryType::CatGroup] = PoolType::Unique;
    m_mapCategoryPoolType[CategoryType::CatLbc] = PoolType::Mix;
    m_mapCategoryPoolType[CategoryType::CatResult] = PoolType::Unique;
}

void DatabaseSession::setNotifyDispatcher(NotifyDispatcher &disp)
{
    m_observer = disp.attach(this, &DatabaseSession::onNotify);
}

void DatabaseSession::add(CategoryType cat)
{
    PoolType pt = static_cast<PoolType>(m_mapCategoryPoolType.at(cat));
    auto &pPool = m_mapCategoryPool[cat];
    if (!pPool)
    {
        if (pt == PoolType::Unique)
            pPool = std::make_unique<PoolUnique>(cat, &m_chunkCursor, m_iChunkBytes, m_bLazy);
        else
            pPool = std::make_unique<PoolMix>(cat, &m_chunkCursor, m_iChunkBytes, m_bLazy);
    }
}

Pool *DatabaseSession::ensure(CategoryType cat)
{
    if (!m_transaction.isOpen())
        return nullptr;

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

Pool *DatabaseSession::checkOutPool(CategoryType cat)
{
    // Normal user path requires an open transaction; engine paths (undo/redo)
    // run under m_engineDepth>0 and are allowed to mutate without a user TX.
    const bool bPass = (m_iTransactionPassCount > 0);
    if (!m_transaction.isOpen() && !bPass)
        return nullptr;

    auto it = m_mapCategoryPool.find(cat);
    if (it == m_mapCategoryPool.end())
    {
        if (!bPass)
            return nullptr;
        // Engine path may need to recreate the store (e.g. undo of erase).
        add(cat);
        it = m_mapCategoryPool.find(cat);
        if (it == m_mapCategoryPool.end()) return nullptr;
    }
    return it->second.get();
}

const Pool *DatabaseSession::getPool(CategoryType cat) const
{
    auto it = m_mapCategoryPool.find(cat);
    return (it == m_mapCategoryPool.end()) ? nullptr : it->second.get();
}

PoolUnique *DatabaseSession::checkOutPoolUnique(CategoryType cat)
{
    if (auto *p = checkOutPool(cat))
        return dynamic_cast<PoolUnique *>(p);
    return nullptr;
}

PoolMix *DatabaseSession::checkOutPoolMix(CategoryType cat)
{
    if (auto *p = checkOutPool(cat))
        return dynamic_cast<PoolMix *>(p);
    return nullptr;
}

const PoolUnique *DatabaseSession::getPoolUnique(CategoryType cat) const
{
    if (auto *p = getPool(cat))
        return dynamic_cast<const PoolUnique *>(p);
    return nullptr;
}
const PoolMix *DatabaseSession::getPoolMix(CategoryType cat) const
{
    if (auto *p = getPool(cat))
        return dynamic_cast<const PoolMix *>(p);
    return nullptr;
}

bool DatabaseSession::erase(HCursor *cur)
{
    if (!cur)
        return false;
    if (!m_transaction.isOpen())
        return false;

    const ItemType ti = cur->type();
    const Id id = cur->id();
    const uint16_t var = cur->variant();
    CategoryType cat = HItemManager::getInstance().getCategory(ti);

    Pool *store = checkOutPool(cat);
    if (!store)
        return false;

    // Mirror clearCategory()'s per-item scheduling logic
    auto &txmap = m_transaction.get_current_transaction();
    Key k{ti, id};
    if (auto it = txmap.find(k); it != txmap.end() && it->second.type == TransactionManager::TransactionType::Emplace)
    {
        // Emplace then erase in same tx → cancel and physical erase now
        txmap.erase(it);
        (void)store->eraseRaw(ti, id);
        return true;
    }

    // If not already Modify, capture BEFORE snapshot
    std::string before;
    if (auto it = txmap.find(k); it == txmap.end() || it->second.type != TransactionManager::TransactionType::Modify)
        if (auto const *p = store->getRawConst(ti, id))
            before = HItemManager::getInstance().captureTransaction(ItemTypeVariant{ti, var}, p, this);

    auto &op = txmap[k];
    if (op.type == TransactionManager::TransactionType{})
    {
        op.type = TransactionManager::TransactionType::Erase;
        op.itemType = ItemTypeVariant{ti, var};
        op.id = id;
        op.payloadBefore = std::move(before);
    }
    else if (op.type == TransactionManager::TransactionType::Modify)
    {
        op.type = TransactionManager::TransactionType::Erase; // promote
        op.payloadAfter.clear();
        if (op.payloadBefore.empty())
            op.payloadBefore = std::move(before);
    }
    return true;
}

bool DatabaseSession::erase(ItemType ti, Id id)
{
    if (!m_transaction.isOpen())
        return false;
    if (auto *cur = getCursor(ti, id))
        return erase(cur);
    return false;
}

HCursor *DatabaseSession::getCursor(ItemType ti, Id id)
{
    CategoryType cat = HItemManager::getInstance().getCategory(ti);
    if (auto const *store = getPool(cat))
    {
        if (auto const *p = store->getRawConst(ti, id))
        {
            return const_cast<HCursor *>(static_cast<const HItem *>(p)->getCursor());
        }
    }
    return nullptr;
}

void DatabaseSession::clearCategory(CategoryType cat)
{
    auto it = m_mapCategoryPool.find(cat);
    if (it == m_mapCategoryPool.end())
        return;

    // For safety: require an open transaction to clear data
    if (!m_transaction.isOpen())
        throw std::logic_error("clearCategory requires an open transaction");

    Pool *store = it->second.get();

    // Helper to create / merge an Erase op for (type,id)
    auto schedule_erase = [&](ItemTypeVariant ti, Id id)
    {
        auto &cur = m_transaction.get_current_transaction();
        Key k{ti.type, id};
        auto found = cur.find(k);
        if (found != cur.end() && found->second.type == TransactionManager::TransactionType::Emplace)
        {
            // Emplace then clear → cancel and erase now
            cur.erase(found);
            (void)store->eraseRaw(ti.type, id);
            return;
        }

        auto &op = cur[k];
        if (op.type == TransactionManager::TransactionType{})
        {
            op.type = TransactionManager::TransactionType::Erase;
            op.itemType = ti;
            op.id = id;

            if (auto const *p = store->getRawConst(ti.type, id))
            {
                op.payloadBefore = HItemManager::getInstance()
                                       .captureTransaction(ti, p, this);
            }
        }
        else if (op.type == TransactionManager::TransactionType::Modify)
        {
            // promote Modify -> Erase; BEFORE kept, AFTER irrelevant
            op.type = TransactionManager::TransactionType::Erase;
            op.payloadAfter.clear();
        }
        // If already Erase, nothing to do.
    };

    // Iterate all items in this category and schedule Erase ops
    if (auto *pu = dynamic_cast<PoolUnique *>(store))
    {
        for (HCursor *c : pu->range())
        {
            if (!c)
                continue;
            // schedule_erase(c->stAssoc.eType, c->iId);
            schedule_erase(ItemTypeVariant{c->type(), c->variant()}, c->id());
        }
    }
    else if (auto *pm = dynamic_cast<PoolMix *>(store))
    {
        for (HCursor *c : pm->range())
        {
            if (!c)
                continue;
            // schedule_erase(c->stAssoc.eType, c->iId);
            schedule_erase(ItemTypeVariant{c->type(), c->variant()}, c->id());
        }
    }

    // Do NOT call store->clear() here; erasure will be applied at commit
    // via Pool{Unique,Mix}::updateBytes(Erase) → eraseRaw(...).
}

void DatabaseSession::clear()
{
    if (!m_transaction.isOpen())
        throw std::logic_error("clear requires an open transaction");

    // collect categories first (don’t mutate while iterating)
    std::vector<CategoryType> cats;
    cats.reserve(m_mapCategoryPool.size());
    for (auto &kv : m_mapCategoryPool)
        cats.push_back(kv.first);

    for (CategoryType cat : cats)
        clearCategory(cat);

    // Note: we intentionally keep empty Pool objects alive.
    // The actual object deletion happens at commit (updateBytes→eraseRaw).
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
            if (auto *store = checkOutPool(HItemManager::getInstance().getCategory(op.itemType.type)))
            {
                if (auto const *p = store->getRawConst(op.itemType.type, op.id))
                {
                    op.payloadAfter = HItemManager::getInstance().captureTransaction(op.itemType, p, this);
                }
            }
        }
    }

    // Apply commit direction
    for (auto &[k, op] : cur)
    {
        if (auto *store = checkOutPool(HItemManager::getInstance().getCategory(op.itemType.type)))
            store->updateBytes(op, this);
    }

    m_transaction.commit();
    m_observer.notify(MessageType::DataModified, 0, 0);
}

void DatabaseSession::rollbackTransaction()
{
    if (!m_transaction.isOpen())
        return;

    auto &cur = m_transaction.get_current_transaction();
    for (auto it = cur.begin(); it != cur.end(); ++it)
    {
        if (auto *store = checkOutPool(HItemManager::getInstance().getCategory(it->second.itemType.type)))
            store->restoreBytes(it->second, this);
    }
    m_transaction.rollback();
}

bool DatabaseSession::undo()
{
    // Transaction pass: allow store checkout & creation without a user TX.
    TransactionPass pass(this);

    const TransactionManager::Transaction *tx = nullptr;
    if (m_transaction.undo(tx) && tx)
    {
        for (auto it = tx->ops.rbegin(); it != tx->ops.rend(); ++it)
        {
            const auto &op = *it;
            auto *store = checkOutPool(HItemManager::getInstance().getCategory(op.itemType.type));
            if (!store)
                continue;

            switch (op.type)
            {
            case TransactionManager::TransactionType::Emplace:
                // Undo insert → erase it
                store->restoreBytes(op, this);
                break;
            case TransactionManager::TransactionType::Erase:
                // Undo erase → recreate and apply BEFORE
                (void)undoUpdateBytes(op);
                break;
            case TransactionManager::TransactionType::Modify:
                // Restore BEFORE
                store->restoreBytes(op, this);
                break;
            }
        }
        m_observer.notify(MessageType::DataModified, 0, 0);
        return true;
    }
    return false;
}

bool DatabaseSession::redo()
{
    // Transaction pass: allow store checkout & creation without a user TX.
    TransactionPass pass(this);

    const TransactionManager::Transaction *tx = nullptr;
    if (m_transaction.redo(tx) && tx)
    {
        for (auto &op : tx->ops)
        {
            auto *store = checkOutPool(HItemManager::getInstance().getCategory(op.itemType.type));
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
                store->updateBytes(op, this);
                break;
            case TransactionManager::TransactionType::Modify:
                // Apply AFTER
                store->updateBytes(op, this);
                break;
            }
        }
        m_observer.notify(MessageType::DataModified, 0, 0);
        return true;
    }
    return false;
}

void DatabaseSession::onNotify(MessageType mess, MessageParam a, MessageParam b)
{
    if (mess == MessageType::DataModified)
    {
        // Do something
    }
}

// Recreate an erased object and restore its BEFORE state (used by UNDO of Erase)
bool DatabaseSession::undoUpdateBytes(const TransactionManager::TransactionOperation &op)
{
    CategoryType cat = HItemManager::getInstance().getCategory(op.itemType.type);
    auto *store = checkOutPool(cat);
    if (!store)
        return false;

    HItemCreatorToken tok{};
    if (!store->emplaceRaw(op.itemType, op.id, tok))
        return false;

    if (void *p = store->getRaw(op.itemType.type, op.id))
    {
        HItemManager::getInstance().restoreTransaction(op.itemType, p, op.payloadBefore, this);
        return true;
    }
    return false;
}

// Recreate an inserted object and restore its AFTER state (used by REDO of Emplace)
bool DatabaseSession::redoUpdateBytes(const TransactionManager::TransactionOperation &op)
{
    CategoryType cat = HItemManager::getInstance().getCategory(op.itemType.type);
    auto *store = checkOutPool(cat);
    if (!store)
        return false;

    if (void *p = store->getRaw(op.itemType.type, op.id))
    {
        // already exists: idempotent
        HItemManager::getInstance().restoreTransaction(op.itemType, p, op.payloadAfter, this);
        return true;
    }

    HItemCreatorToken tok{};
    if (!store->emplaceRaw(op.itemType, op.id, tok))
        return false;

    if (void *p2 = store->getRaw(op.itemType.type, op.id))
    {
        HItemManager::getInstance().restoreTransaction(op.itemType, p2, op.payloadAfter, this);
        return true;
    }
    return false;
}
