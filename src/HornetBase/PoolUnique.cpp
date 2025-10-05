#include "HornetBase/PoolUnique.h"
#include "HornetBase/Chunk.h"
#include "HornetBase/HItemManager.h"

PoolUnique::PoolUnique(CategoryType cat,
                       std::size_t chunk_bytes_per_type,
                       bool lazy_first_chunk)
    : Pool(cat), chunk_bytes_per_type_(chunk_bytes_per_type), lazy_first_chunk_(lazy_first_chunk)
{
}

void PoolUnique::clear()
{
    for (auto &kv : pools_)
        kv.second->clear();
}

std::size_t PoolUnique::bytesUsed() const
{
    std::size_t total = 0;
    for (auto const &kv : pools_)
        total += kv.second->bytesUsed();
    return total;
}

std::size_t PoolUnique::count() const
{
    std::size_t total = 0;
    for (auto const &kv : pools_)
        total += kv.second->count();
    return total;
}

void PoolUnique::restoreBytes(TransactionManager::TransactionOperation tx)
{
    switch (tx.type)
    {
    case TransactionManager::TransactionType::Emplace:
        (void)eraseRaw(tx.itemType, tx.id);
        break;
    case TransactionManager::TransactionType::Erase:
        // deferred erase; nothing to undo
        break;
    case TransactionManager::TransactionType::Modify:
        if (void *p = getRaw(tx.itemType, tx.id))
        {
            HItemManager::getInstance().restoreTransaction(tx.itemType, p, tx.payloadBefore);
        }
        break;
    }
}

void PoolUnique::updateBytes(TransactionManager::TransactionOperation tx)
{
    switch (tx.type)
    {
    case TransactionManager::TransactionType::Emplace:
        // already present
        break;
    case TransactionManager::TransactionType::Erase:
        (void)eraseRaw(tx.itemType, tx.id);
        break;
    case TransactionManager::TransactionType::Modify:
        if (void *p = getRaw(tx.itemType, tx.id))
        {
            HItemManager::getInstance().restoreTransaction(tx.itemType, p, tx.payloadAfter);
        }
        break;
    }
}

void *PoolUnique::getRaw(ItemType ti, Id id)
{
    auto it = pools_.find(ti);
    if (it == pools_.end())
        return nullptr;
    return static_cast<ChunkInterface *>(it->second.get())->getRaw(id);
}

const void *PoolUnique::getRawConst(ItemType ti, Id id) const
{
    auto it = pools_.find(ti);
    if (it == pools_.end())
        return nullptr;
    return static_cast<const ChunkInterface *>(it->second.get())->getRawConst(id);
}

bool PoolUnique::eraseRaw(ItemType ti, Id id)
{
    auto it = pools_.find(ti);
    if (it == pools_.end())
        return false;
    return static_cast<ChunkInterface *>(it->second.get())->eraseRaw(id);
}

bool PoolUnique::emplaceRaw(ItemType ti, Id id, HItemCreatorToken tok)
{
    auto it = pools_.find(ti);
    if (it == pools_.end())
        return false;
    return static_cast<ChunkInterface *>(it->second.get())->emplaceRaw(id, tok);
}
