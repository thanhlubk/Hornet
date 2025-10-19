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
// PoolMix: multiple types packed into shared 1 MiB chunks
//  - Keys are (type_index, Id), so same numeric Id can exist per type
//  - Erase leaves holes (no immediate compaction)
//  - Emplace compacts ALL chunks first if tail chunk is full; if still full, allocates one chunk
// --------------------------------------------------------------------------------------
class HORNETBASE_EXPORT PoolMix : public Pool
{
public:
    // Preferred ctor with category
    explicit PoolMix(CategoryType cat, ChunkCursor* chunkCursor, std::size_t chunk_bytes = kDefaultChunkBytes, bool lazy_first_chunk = true);
    // Convenience ctor with default category
    // explicit PoolMix(std::size_t chunk_bytes = kDefaultChunkBytes, bool lazy_first_chunk = true);

    PoolMix(const PoolMix &) = delete;
    PoolMix &operator=(const PoolMix &) = delete;
    PoolMix(PoolMix &&) noexcept = default;
    PoolMix &operator=(PoolMix &&) noexcept = default;
    ~PoolMix() = default; // declare out-of-line (optional)

    struct MixChunk
    {
        std::unique_ptr<unsigned char[]> buf; // unsigned char for pointer arithmetic
        std::size_t capacity = 0;
        std::size_t used = 0; // high watermark (holes allowed)
        std::vector<Key> ids; // live (type,id) in insertion order
        explicit MixChunk(std::size_t cap) : buf(new unsigned char[cap]), capacity(cap), used(0) {}
    };

    struct MixLoc
    {
        std::uint32_t chunk;
        std::uint32_t offset;
        const ItemTypeDescriptor *ops;
    };

    // Insert any T by (T,id); constructs T in-place and returns reference
    template <HItemTemplate T, class... Args>
    bool emplace(Id id, Args &&...args)
    {
        Key key{ItemTypeOf<T>, id};
        if (index_.find(key) != index_.end())
        {
            throw std::runtime_error("(T,id) already exists");
        }
        // const TypeOps& ops = TypeOps::for_type<T>();
        const ItemTypeDescriptor *desc = HItemManager::getInstance().descriptor(ItemTypeOf<T>);
        if (!desc)
            throw std::runtime_error("Type not registered in HItemManager");

        if (chunks_.empty())
            new_chunk();

        MixChunk *c = &chunks_.back();
        std::size_t aligned = alignUp(c->used, desc->align);

        if (aligned + desc->size > c->capacity)
        {
            // 1) Try to reclaim holes globally
            compact_all();

            // 2) Re-evaluate after compaction
            if (chunks_.empty())
                new_chunk();
            c = &chunks_.back();
            aligned = alignUp(c->used, desc->align);

            // 3) If still no room, grow by one chunk
            if (aligned + desc->size > c->capacity)
            {
                new_chunk();
                c = &chunks_.back();
                aligned = alignUp(c->used, desc->align);
            }
        }

        void *ptr = static_cast<void *>(c->buf.get() + aligned);
        HCursor *pCursor = m_pChunkCursor->emplace();
        T *obj = ::new (ptr) T(id, pCursor, std::forward<Args>(args)...);
        (void)obj;

        c->used = aligned + desc->size;
        c->ids.push_back(key);
        index_[key] = MixLoc{static_cast<std::uint32_t>(chunks_.size() - 1),
                             static_cast<std::uint32_t>(aligned), desc};
        ++count_;
        return true;
        // return *reinterpret_cast<T*>(ptr);
    }

    // Lookup by (T,id)
    template <HItemTemplate T>
    T *get(Id id)
    {
        Key key{ItemTypeOf<T>, id};
        auto it = index_.find(key);
        if (it == index_.end())
            return nullptr;
        const MixLoc &loc = it->second;
        return std::launder(reinterpret_cast<T *>(chunks_[loc.chunk].buf.get() + loc.offset));
    }

    template <HItemTemplate T>
    const T *get(Id id) const
    {
        return const_cast<PoolMix *>(this)->template get<T>(id);
    }

    // Erase by (T,id); destroy object and LEAVE A HOLE (no compaction here)
    template <HItemTemplate T>
    bool erase(Id id)
    {
        Key key{ItemTypeOf<T>, id};
        auto it = index_.find(key);
        if (it == index_.end())
            return false;
        MixLoc loc = it->second;
        MixChunk &c = chunks_[loc.chunk];

        // Erase cursor first
        if (auto *obj = get<T>(it->first.id))
            m_pChunkCursor->erase(obj->getCursor());

        // Destroy object
        void *ptr = static_cast<void *>(c.buf.get() + loc.offset);
        loc.ops->destroy(ptr);

        index_.erase(it);
        --count_;

        // remove (type,id) from the chunk s list (byte hole remains)
        auto &ids = c.ids;
        for (std::size_t i = 0; i < ids.size(); ++i)
        {
            if (ids[i] == key)
            {
                ids.erase(ids.begin() + i);
                break;
            }
        }
        return true;
    }

    // Iterate all live objects of type T (unordered)
    template <HItemTemplate T, class F>
    void for_each(F &&f)
    {
        ItemType ti = ItemTypeOf<T>;
        for (auto &kv : index_)
        {
            if (kv.first.type != ti)
                continue;
            const MixLoc &loc = kv.second;
            T *obj = std::launder(reinterpret_cast<T *>(chunks_[loc.chunk].buf.get() + loc.offset));
            f(kv.first.id, *obj);
        }
    }

    // Optional: iterate in memory order (by chunk, then offset) for T
    template <HItemTemplate T, class F>
    void for_each_in_memory_order(F &&f)
    {
        ItemType ti = ItemTypeOf<T>;
        struct Item
        {
            std::uint32_t chunk;
            std::uint32_t offset;
            Id id;
        };
        std::vector<Item> items;
        items.reserve(index_.size());
        for (auto &kv : index_)
        {
            if (kv.first.type != ti)
                continue;
            items.push_back({kv.second.chunk, kv.second.offset, kv.first.id});
        }
        std::sort(items.begin(), items.end(), [](const Item &a, const Item &b)
                  {
            if (a.chunk != b.chunk) return a.chunk < b.chunk;
            return a.offset < b.offset; });
        for (auto &it : items)
        {
            T *obj = std::launder(reinterpret_cast<T *>(chunks_[it.chunk].buf.get() + it.offset));
            f(it.id, *obj);
        }
    }

    // ---- Range-for iteration over all objects of type T in PoolMix ----
    // Iterates in hash-map order (unordered). For memory order, see optional helper below.
    // ---- Range-for iteration over all objects of type T in PoolMix, yielding T* / const T* ----
    template <HItemTemplate T, bool isConst>
    class Iterator
    {
        using Owner = std::conditional_t<isConst, const PoolMix, PoolMix>;
        using MapIter = std::conditional_t<isConst,
                                           typename std::unordered_map<Key, MixLoc, KeyHash>::const_iterator,
                                           typename std::unordered_map<Key, MixLoc, KeyHash>::const_iterator>; // const is fine for both
    public:
        using difference_type = std::ptrdiff_t;
        using value_type = std::conditional_t<isConst, const T *, T *>;
        using reference = value_type;
        using pointer = value_type;
        using iterator_category = std::forward_iterator_tag;

        Iterator(MapIter it, MapIter end, Owner *owner)
            : it_(it), end_(end), owner_(owner), ti_(ItemTypeOf<T>)
        {
            advance_to_T();
        }

        reference operator*() const
        {
            const MixLoc &loc = it_->second;
            auto base = owner_->chunks_[loc.chunk].buf.get() + loc.offset;
            return std::launder(reinterpret_cast<value_type>(base));
        }

        Iterator &operator++()
        {
            ++it_;
            advance_to_T();
            return *this;
        }
        bool operator==(const Iterator &rhs) const { return it_ == rhs.it_; }
        bool operator!=(const Iterator &rhs) const { return it_ != rhs.it_; }

    private:
        void advance_to_T()
        {
            while (it_ != end_ && it_->first.type != ti_)
                ++it_;
        }

        MapIter it_, end_;
        Owner *owner_;
        ItemType ti_;
    };

    template <HItemTemplate T, bool isConst>
    class Range
    {
        using Owner = std::conditional_t<isConst, const PoolMix, PoolMix>;
        using Iter = Iterator<T, isConst>;

    public:
        explicit Range(Owner *owner) : owner_(owner) {}
        Iter begin() const { return Iter(owner_->index_.cbegin(), owner_->index_.cend(), owner_); }
        Iter end() const { return Iter(owner_->index_.cend(), owner_->index_.cend(), owner_); }

    private:
        Owner *owner_;
    };

    template <HItemTemplate T>
    Range<T, false> range() { return Range<T, false>(this); }

    template <HItemTemplate T>
    Range<T, true> range() const { return Range<T, true>(this); }

    // ================== Type-erased iteration over ALL objects (unordered) ==================
    class IteratorRaw
    {
        using MapIter = std::unordered_map<Key, MixLoc, KeyHash>::const_iterator; // const iterators
    public:
        using difference_type = std::ptrdiff_t;
        using value_type = HCursor *; // always HCursor*
        using reference = HCursor *;

        IteratorRaw(MapIter it, MapIter ed, const PoolMix *owner)
            : it_(it), ed_(ed), owner_(owner) {}

        HCursor *operator*() const
        {
            const MixLoc &l = it_->second;
            const void *p = static_cast<const void *>(owner_->chunks_[l.chunk].buf.get() + l.offset);
            auto *item = static_cast<const HItem *>(p);
            // HItem::getCursor() const → const HCursor* ; cast away const for your read-only cursor API
            return const_cast<HCursor *>(item->getCursor());
        }
        IteratorRaw &operator++()
        {
            ++it_;
            return *this;
        }
        bool operator!=(const IteratorRaw &o) const { return it_ != o.it_; }

    private:
        MapIter it_, ed_;
        const PoolMix *owner_;
    };

    class RangeRaw
    {
    public:
        explicit RangeRaw(const PoolMix *owner) : owner_(owner) {}
        IteratorRaw begin() const { return IteratorRaw(owner_->index_.cbegin(), owner_->index_.cend(), owner_); }
        IteratorRaw end() const { return IteratorRaw(owner_->index_.cend(), owner_->index_.cend(), owner_); }

    private:
        const PoolMix *owner_;
    };

    RangeRaw range() const { return RangeRaw(this); }

    // ---- Pool overrides ----
    void clear() override;

    std::size_t bytesUsed() const override;

    std::size_t count() const override;

    void restoreBytes(TransactionManager::TransactionOperation tx, DatabaseSession *pDb) override;
    void updateBytes(TransactionManager::TransactionOperation tx, DatabaseSession *pDb) override;

    void *getRaw(ItemType ti, Id id) override;
    const void *getRawConst(ItemType ti, Id id) const override;
    bool eraseRaw(ItemType ti, Id id) override;
    bool emplaceRaw(ItemType ti, Id id, HItemCreatorToken tok);

private:
    void new_chunk();

    // Global compaction:
    //  - packs ALL live objects tightly into as few chunks as needed
    //  - updates every index entry & per-chunk id list
    void compact_all();

    std::size_t chunk_bytes_;
    bool lazy_first_chunk_ = true;
    std::vector<MixChunk> chunks_;
    std::unordered_map<Key, MixLoc, KeyHash> index_;
    std::size_t count_ = 0;
};