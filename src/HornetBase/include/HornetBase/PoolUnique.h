#pragma once
#include "Pool.h"
#include "Chunk.h"
#include <memory>
#include <unordered_map>
#include <algorithm>
#include <utility>
#include <stdexcept>

// --------------------------------------------------------------------------------------
// Database: owns one ChunkItem<T> per type
// --------------------------------------------------------------------------------------
class HORNETBASE_EXPORT PoolUnique : public Pool
{
public:
    explicit PoolUnique(CategoryType cat,
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
            auto up = std::make_unique<ChunkItem<T>>(chunk_bytes_per_type_, lazy_first_chunk_);
            auto *raw = up.get();
            pools_[ti] = std::move(up);
            return *raw;
        }
        return *static_cast<ChunkItem<T> *>(pools_[ti].get());
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

    class IteratorRaw
    {
    public:
        using PoolsIter = typename std::unordered_map<ItemType, std::unique_ptr<ChunkInterface>>::iterator;
        using difference_type = std::ptrdiff_t;
        using value_type = void *;
        using reference = void *;

        IteratorRaw(PoolUnique *db, PoolsIter pit, PoolsIter pend)
            : db_(db), pit_(pit), pend_(pend)
        {
            advance_to_next_valid();
        }

        reference operator*() const { return anyPtr_; }

        IteratorRaw &operator++()
        {
            // advance current cursor
            if (cursor_)
            {
                Id id;
                void *p = nullptr;
                if (cursor_->next(id, p))
                {
                    // cur_.id = id; cur_.ptr = p;
                    anyPtr_ = p;

                    // cur_.type remains same while in the same pool
                    return *this;
                }
            }
            // otherwise advance to next pool
            ++pit_;
            advance_to_next_valid();
            return *this;
        }

        bool operator!=(const IteratorRaw &o) const { return pit_ != o.pit_ || end_ != o.end_; }

    private:
        void advance_to_next_valid()
        {
            // Walk pools until we find one that yields at least one element
            while (pit_ != pend_)
            {
                auto *ep = pit_->second.get();
                cursor_ = ep->newCursor();
                Id id;
                void *p = nullptr;
                if (cursor_ && cursor_->next(id, p))
                {
                    // cur_.type = pit_->first;
                    // cur_.id = id;
                    // cur_.ptr = p;
                    anyPtr_ = p;
                    end_ = false;
                    return;
                }
                ++pit_;
            }
            // no pools or all empty
            end_ = true;
        }

        PoolUnique *db_ = nullptr;
        PoolsIter pit_, pend_;
        std::unique_ptr<ChunkInterface::CursorRaw> cursor_;

        void *anyPtr_ = nullptr;
        // AnyRef cur_{ std::type_index(typeid(void)), 0, nullptr };
        bool end_ = false;
    };

    class RangeRaw
    {
    public:
        explicit RangeRaw(PoolUnique *db) : db_(db) {}
        IteratorRaw begin() { return IteratorRaw(db_, db_->pools_.begin(), db_->pools_.end()); }
        IteratorRaw end() { return IteratorRaw(db_, db_->pools_.end(), db_->pools_.end()); }

    private:
        PoolUnique *db_;
    };

    RangeRaw range() { return RangeRaw(this); }

    // ---- Pool overrides ----
    void clear() override;
    std::size_t bytesUsed() const override;
    std::size_t count() const override;

    // transaction
    void restoreBytes(TransactionManager::TransactionOperation tx) override;
    void updateBytes(TransactionManager::TransactionOperation tx) override;

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