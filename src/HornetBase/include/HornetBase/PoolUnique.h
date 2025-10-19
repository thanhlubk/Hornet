#pragma once
#include "Pool.h"
#include "Chunk.h"
#include <memory>
#include <unordered_map>
#include <algorithm>
#include <utility>
#include <stdexcept>

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
    {
        return pool<T>().emplace(id, std::forward<Args>(args)...);
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
    bool emplaceRaw(ItemType ti, Id id, HItemCreatorToken tok) override;

private:
    std::size_t chunk_bytes_per_type_ = kDefaultChunkBytes;
    bool lazy_first_chunk_ = true;

    // Map: ItemType -> ChunkInterface
    std::unordered_map<ItemType, std::unique_ptr<ChunkInterface>> pools_;
};