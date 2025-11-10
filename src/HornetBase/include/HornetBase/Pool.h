#pragma once
#include "HBaseDef.h"
#include "TransactionManager.h"
#include "Chunk.h"
// --------------------------------------------------------------------------------------
// Pool: common base for PoolUnique & PoolMix with category + type erasure
// --------------------------------------------------------------------------------------

class DatabaseSession; // forward declare

class Pool
{
public:
    explicit Pool(CategoryType cat, ChunkCursor *chunkCursor) : m_eCategory(cat), m_pChunkCursor(chunkCursor) {}
    virtual ~Pool() = default;

    CategoryType category() const { return m_eCategory; }

    // type-erased access
    virtual void *getRaw(ItemType ti, Id id) = 0;
    virtual const void *getRawConst(ItemType ti, Id id) const = 0;
    virtual bool eraseRaw(ItemType ti, Id id) = 0;
    virtual bool emplaceRaw(ItemTypeVariant ti, Id id, HItemCreatorToken tok) = 0;

    // management / stats
    virtual void clear() = 0;
    virtual std::size_t bytesUsed() const = 0;
    virtual std::size_t count() const = 0;

    // transaction
    virtual void restoreBytes(TransactionManager::TransactionOperation transaction, DatabaseSession* pDb) = 0;
    virtual void updateBytes(TransactionManager::TransactionOperation transaction, DatabaseSession *pDb) = 0;

    // typed convenience wrappers (usable even via Pool*)
    template <HItemTemplate T>
    T *get(Id id)
    {
        return static_cast<T *>(getRaw(ItemTypeOf<T>, id));
    }

    template <HItemTemplate T>
    const T *get(Id id) const
    {
        return static_cast<const T *>(getRawConst(ItemTypeOf<T>, id));
    }

    template <HItemTemplate T>
    bool erase(Id id)
    {
        return eraseRaw(ItemTypeOf<T>, id);
    }

private:
    CategoryType m_eCategory;
    
protected:
    ChunkCursor *m_pChunkCursor;
};