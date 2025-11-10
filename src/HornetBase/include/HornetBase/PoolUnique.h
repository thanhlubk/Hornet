#pragma once
#include "Pool.h"
#include "Chunk.h"
#include <memory>
#include <unordered_map>
#include <algorithm>
#include <utility>
#include <stdexcept>
#include "HIElement.h"

struct ItemTypeDescriptor;

// --------------------------------------------------------------------------------------
// Database: owns one ChunkItem<T> per type
// --------------------------------------------------------------------------------------
class HORNETBASE_EXPORT PoolUnique : public Pool
{
public:
    explicit PoolUnique(CategoryType cat, ChunkCursor *chunkCursor,
                        std::size_t chunk_bytes_per_type = kDefaultChunkBytes,
                        bool lazy_first_chunk = true);

    PoolUnique(const PoolUnique &) = delete;
    PoolUnique &operator=(const PoolUnique &) = delete;
    PoolUnique(PoolUnique &&) noexcept = default;
    PoolUnique &operator=(PoolUnique &&) noexcept = default;
    ~PoolUnique() = default; // declare out-of-line (optional but recommended for DLL hygiene)

    template <HItemTemplate T>
    ChunkItem<T> &pool()
    {
        ItemType ti = ItemTypeOf<T>;
        auto it = pools_.find(ti);
        if (it == pools_.end())
        {
            auto up = std::make_unique<ChunkItem<T>>(chunk_bytes_per_type_, lazy_first_chunk_, m_pChunkCursor);
            auto *raw = up.get();
            pools_[ti] = std::move(up);
            return *raw;
        }
        return *static_cast<ChunkItem<T> *>(pools_[ti].get());
    }

    template <HItemTemplate T>
    const ChunkItem<T> &pool() const
    {
        ItemType ti = ItemTypeOf<T>;
        auto it = pools_.find(ti);
        if (it == pools_.end())
            throw std::runtime_error("No pool for requested type");
        return *static_cast<const ChunkItem<T> *>(it->second.get());
    }

    template <HItemTemplate T, class... Args>
    bool emplace(Id id, Args &&...args)
        requires(!std::same_as<std::remove_cvref_t<T>, HIElement>)
    {
        return pool<T>().emplace(id, std::forward<Args>(args)...);
    }

    template <HItemTemplate T, class... Args>
    bool emplace(Id id, HItemCreatorToken tok, Args &&...args)
        requires std::same_as<std::remove_cvref_t<T>, HIElement>
    { 
        return false;
    }

    template <HItemTemplate T>
    T *get(Id id)
    {
        return pool<T>().get(id);
    }

    template <HItemTemplate T>
    bool erase(Id id)
    {
        return pool<T>().erase(id);
    }

    template <HItemTemplate T>
    void compact()
    {
        pool<T>().compact();
    }

    // Range-for over all live T in this Database (unordered by hash map of the pool)
    template <HItemTemplate T>
    typename ChunkItem<T>::Range range()
    {
        return typename ChunkItem<T>::Range{this->pool<T>()};
    }

    template <HItemTemplate T>
    typename ChunkItem<T>::RangeConst range() const
    {
        return typename ChunkItem<T>::RangeConst{this->pool<T>()};
    }

    class IteratorRaw
    {
        using PoolsIt = std::unordered_map<ItemType, std::unique_ptr<ChunkInterface>>::const_iterator;

    public:
        using difference_type = std::ptrdiff_t;
        using value_type = HCursor *;
        using reference = HCursor *;

        IteratorRaw(const PoolUnique *db, PoolsIt pit, PoolsIt pend)
            : db_(db), pit_(pit), pend_(pend) { advance_to_next_valid(); }

        HCursor *operator*() const { return cur_; }

        IteratorRaw &operator++()
        {
            if (cursor_)
            {
                Id id;
                void *p = nullptr;
                if (cursor_->next(id, p))
                {
                    cur_ = const_cast<HCursor *>(static_cast<const HItem *>(p)->getCursor());
                    return *this;
                }
            }
            ++pit_;
            advance_to_next_valid();
            return *this;
        }
        bool operator!=(const IteratorRaw &o) const { return pit_ != o.pit_ || end_ != o.end_; }

    private:
        void advance_to_next_valid()
        {
            while (pit_ != pend_)
            {
                // If ChunkInterface offers newCursor() const, use it. Otherwise:
                ChunkInterface *ep = const_cast<ChunkInterface *>(pit_->second.get());
                cursor_ = ep->newCursor();
                Id id;
                void *p = nullptr;
                if (cursor_ && cursor_->next(id, p))
                {
                    cur_ = const_cast<HCursor *>(static_cast<const HItem *>(p)->getCursor());
                    end_ = false;
                    return;
                }
                ++pit_;
            }
            end_ = true;
        }

        const PoolUnique *db_ = nullptr;
        PoolsIt pit_, pend_;
        std::unique_ptr<ChunkInterface::CursorRaw> cursor_;
        HCursor *cur_ = nullptr;
        bool end_ = false;
    };

    class RangeRaw
    {
    public:
        explicit RangeRaw(const PoolUnique *db) : db_(db) {}
        IteratorRaw begin() const { return IteratorRaw(db_, db_->pools_.cbegin(), db_->pools_.cend()); }
        IteratorRaw end() const { return IteratorRaw(db_, db_->pools_.cend(), db_->pools_.cend()); }

    private:
        const PoolUnique *db_;
    };

    RangeRaw range() const { return RangeRaw(this); }

    // ---- Pool overrides ----
    void clear() override;
    std::size_t bytesUsed() const override;
    std::size_t count() const override;

    // transaction
    void restoreBytes(TransactionManager::TransactionOperation tx, DatabaseSession *pDb) override;
    void updateBytes(TransactionManager::TransactionOperation tx, DatabaseSession *pDb) override;

    void *getRaw(ItemType ti, Id id) override;
    const void *getRawConst(ItemType ti, Id id) const override;
    bool eraseRaw(ItemType ti, Id id) override;
    bool emplaceRaw(ItemTypeVariant ti, Id id, HItemCreatorToken tok) override;

private:
    std::size_t chunk_bytes_per_type_ = kDefaultChunkBytes;
    bool lazy_first_chunk_ = true;

    // Map: ItemType -> ChunkInterface
    std::unordered_map<ItemType, std::unique_ptr<ChunkInterface>> pools_;
};

#if 0 // cpp content
#include "HornetBase/PoolUnique.h"
#include "HornetBase/Chunk.h"
#include "HornetBase/HItemManager.h"

PoolUnique::PoolUnique(CategoryType cat, ChunkCursor *chunkCursor,
                       std::size_t chunk_bytes_per_type,
                       bool lazy_first_chunk)
    : Pool(cat, chunkCursor), chunk_bytes_per_type_(chunk_bytes_per_type), lazy_first_chunk_(lazy_first_chunk)
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

void PoolUnique::restoreBytes(TransactionManager::TransactionOperation tx, DatabaseSession *pDb)
{
    switch (tx.type)
    {
    case TransactionManager::TransactionType::Emplace:
        (void)eraseRaw(tx.itemType.type, tx.id);
        break;
    case TransactionManager::TransactionType::Erase:
        // deferred erase; nothing to undo
        break;
    case TransactionManager::TransactionType::Modify:
        if (void *p = getRaw(tx.itemType.type, tx.id))
        {
            HItemManager::getInstance().restoreTransaction(tx.itemType, p, tx.payloadBefore, pDb);
        }
        break;
    }
}

void PoolUnique::updateBytes(TransactionManager::TransactionOperation tx, DatabaseSession *pDb)
{
    switch (tx.type)
    {
    case TransactionManager::TransactionType::Emplace:
        // already present
        break;
    case TransactionManager::TransactionType::Erase:
        (void)eraseRaw(tx.itemType.type, tx.id);
        break;
    case TransactionManager::TransactionType::Modify:
        if (void *p = getRaw(tx.itemType.type, tx.id))
        {
            HItemManager::getInstance().restoreTransaction(tx.itemType, p, tx.payloadAfter, pDb);
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

bool PoolUnique::emplaceRaw(ItemTypeVariant ti, Id id, HItemCreatorToken tok)
{
    auto it = pools_.find(ti.type);
    if (it == pools_.end())
        return false;
    return static_cast<ChunkInterface *>(it->second.get())->emplaceRaw(id, tok);
}

#endif