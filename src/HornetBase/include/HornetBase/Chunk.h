#pragma once
#include "HBaseDef.h"
#include "HItemManager.h"
#include <cstdint>
#include <cstddef> // std::byte
#include <memory>
#include <new>
#include <stdexcept>
#include <unordered_map>
#include <type_traits>

// --------------------------------------------------------------------------------------
// Chunk interfaces (type-erased)
// --------------------------------------------------------------------------------------
struct ChunkInterface
{
    virtual ~ChunkInterface() = default;

    virtual void clear() = 0;
    virtual std::size_t bytesUsed() const = 0;
    virtual std::size_t count() const = 0;

    virtual void *getRaw(Id id) = 0;
    virtual const void *getRawConst(Id id) const = 0;
    virtual bool eraseRaw(Id id) = 0;
    virtual bool emplaceRaw(Id id, HItemCreatorToken tok) = 0;

    virtual ItemType type() const = 0;

    // Type-erased cursor for lazy iteration
    struct CursorRaw
    {
        virtual ~CursorRaw() = default;
        // Advance to next item; returns false when finished.
        // On success, writes (id, ptr) of the current element.
        virtual bool next(Id &id_out, void *&ptr_out) = 0;
    };
    // Create a new cursor positioned at the beginning of this chunk.
    virtual std::unique_ptr<CursorRaw> newCursor() = 0;
};

// --------------------------------------------------------------------------------------
// Low-level chunk for per-type chunks (ChunkItem<T>)
// --------------------------------------------------------------------------------------
constexpr std::size_t kDefaultChunkBytes = 1u << 20; // 1 MiB

struct Chunk
{
    std::unique_ptr<std::byte[]> buf;
    std::size_t capacity = 0; // bytes
    std::size_t used = 0;     // bytes

    explicit Chunk(std::size_t cap = kDefaultChunkBytes)
        : buf(new std::byte[cap]), capacity(cap), used(0)
    {
    }

    std::size_t space() const { return capacity - used; }
};

struct Location
{
    std::uint32_t chunk = 0;  // chunk index
    std::uint32_t offset = 0; // byte offset within chunk
};

// --------------------------------------------------------------------------------------
// Per-type arena: ChunkItem<T> (used by Database)
// - 1 MiB chunks per T, append-packed, hole reuse (LIFO), lazy-first-chunk option
// --------------------------------------------------------------------------------------
template <class T>
class ChunkItem final : public ChunkInterface
{
    static_assert(!std::is_pointer_v<T>, "Chunk should store objects, not pointers");

public:
    using value_type = T;

    explicit ChunkItem(std::size_t chunk_bytes = kDefaultChunkBytes, bool lazy_first_chunk = true)
        : m_iChunkBytes(chunk_bytes), m_bLazyFirstChunk(lazy_first_chunk)
    {
        if ((m_iChunkBytes & (alignof(T) - 1)) != 0)
        {
            m_iChunkBytes = alignUp(m_iChunkBytes, alignof(T));
        }
        if (!m_bLazyFirstChunk)
        {
            allocateNewChunk(); // eager allocation
        }
    }

    ChunkItem(const ChunkItem &) = delete;
    ChunkItem &operator=(const ChunkItem &) = delete;
    ChunkItem(ChunkItem &&) = default;
    ChunkItem &operator=(ChunkItem &&) = default;

    template <class... Args>
    bool emplace(Id id, Args &&...args)
    {
        if (m_mapIndex.find(id) != m_mapIndex.end())
        {
            throw std::runtime_error("ID already exists for this type");
            return false;
        }

        // Create first chunk lazily if none exist
        if (m_vecChunk.empty())
            allocateNewChunk();

        // Reuse hole if available
        if (!m_vecFreeLocation.empty())
        {
            Location loc = m_vecFreeLocation.back();
            m_vecFreeLocation.pop_back();
            void *ptr = reinterpret_cast<void *>(m_vecChunk[loc.chunk].buf.get() + loc.offset);
            T *obj = ::new (ptr) T(id, std::forward<Args>(args)...);
            m_mapIndex[id] = loc;
            ++m_iCount;
            return true;
            // return *obj;
        }

        const std::size_t obj_align = alignof(T);
        const std::size_t obj_size = sizeof(T);

        Chunk *c = &m_vecChunk.back();
        std::size_t aligned = alignUp(c->used, obj_align);
        if (aligned + obj_size > c->capacity)
        {
            allocateNewChunk();
            c = &m_vecChunk.back();
            aligned = alignUp(c->used, obj_align);
        }

        void *ptr = reinterpret_cast<void *>(c->buf.get() + aligned);
        T *obj = ::new (ptr) T(id, std::forward<Args>(args)...);
        c->used = aligned + obj_size;

        Location loc{static_cast<std::uint32_t>(m_vecChunk.size() - 1),
                     static_cast<std::uint32_t>(aligned)};
        m_mapIndex[id] = loc;
        ++m_iCount;
        return true;
        // return *obj;
    }

    T *get(Id id)
    {
        auto it = m_mapIndex.find(id);
        if (it == m_mapIndex.end())
            return nullptr;
        const Location &loc = it->second;
        return std::launder(reinterpret_cast<T *>(m_vecChunk[loc.chunk].buf.get() + loc.offset));
    }
    const T *get(Id id) const { return const_cast<ChunkItem *>(this)->get(id); }

    bool erase(Id id)
    {
        auto it = m_mapIndex.find(id);
        if (it == m_mapIndex.end())
            return false;
        Location loc = it->second;
        T *obj = std::launder(reinterpret_cast<T *>(m_vecChunk[loc.chunk].buf.get() + loc.offset));
        obj->~T();
        m_mapIndex.erase(it);
        --m_iCount;
        m_vecFreeLocation.push_back(loc);
        return true;
    }

    void compact()
    {
        static_assert(std::is_move_constructible_v<T> || std::is_copy_constructible_v<T>,
                      "T must be move- or copy-constructible for compaction");

        std::vector<Chunk> new_chunks;
        if (!m_mapIndex.empty())
            new_chunks.emplace_back(m_iChunkBytes);

        std::unordered_map<Id, Location> new_index;
        new_index.reserve(m_mapIndex.size());

        auto move_into = [&](Id id, T *src_obj)
        {
            Chunk &nc = new_chunks.back();
            std::size_t aligned = alignUp(nc.used, alignof(T));
            if (aligned + sizeof(T) > nc.capacity)
            {
                new_chunks.emplace_back(m_iChunkBytes);
                aligned = 0;
            }
            void *dest = reinterpret_cast<void *>(new_chunks.back().buf.get() + aligned);
            T *dst_obj = ::new (dest) T(std::move(*src_obj));
            src_obj->~T();
            new_chunks.back().used = aligned + sizeof(T);
            new_index[id] = Location{static_cast<std::uint32_t>(new_chunks.size() - 1),
                                     static_cast<std::uint32_t>(aligned)};
            (void)dst_obj;
        };

        for (auto &kv : m_mapIndex)
        {
            Id id = kv.first;
            const Location &loc = kv.second;
            T *obj = std::launder(reinterpret_cast<T *>(m_vecChunk[loc.chunk].buf.get() + loc.offset));
            move_into(id, obj);
        }

        m_vecChunk.swap(new_chunks);
        m_mapIndex.swap(new_index);
        m_vecFreeLocation.clear();
    }

    template <class F>
    void for_each(F &&f)
    {
        for (auto &kv : m_mapIndex)
        {
            T *obj = get(kv.first);
            f(kv.first, *obj);
        }
    }

    // ---- Range-for iteration over all T in this chunk (unordered by hash map) ----
    class Iterator
    {
    public:
        using MapIter = typename std::unordered_map<Id, Location>::iterator;
        using difference_type = std::ptrdiff_t;
        using value_type = T;
        using reference = T &;
        using pointer = T *;
        using iterator_category = std::forward_iterator_tag;

        Iterator(MapIter it, ChunkItem *owner) : it_(it), owner_(owner) {}

        reference operator*() const
        {
            const Location &loc = it_->second;
            return *std::launder(reinterpret_cast<T *>(owner_->m_vecChunk[loc.chunk].buf.get() + loc.offset));
        }
        pointer operator->() const { return std::addressof(**this); }

        Iterator &operator++()
        {
            ++it_;
            return *this;
        }
        bool operator==(const Iterator &rhs) const { return it_ == rhs.it_; }
        bool operator!=(const Iterator &rhs) const { return it_ != rhs.it_; }

    private:
        MapIter it_;
        ChunkItem *owner_;
    };

    Iterator begin() { return Iterator(m_mapIndex.begin(), this); }
    Iterator end() { return Iterator(m_mapIndex.end(), this); }

    // Small helper so Database can expose a range<T>() that forwards to chunk.begin()/end()
    struct Range
    {
        ChunkItem &p;
        Iterator begin() { return p.begin(); }
        Iterator end() { return p.end(); }
    };

    // ---- ChunkInterface overrides ----
    void *getRaw(Id id) override
    {
        return static_cast<void *>(get(id));
    }
    const void *getRawConst(Id id) const override
    {
        return static_cast<const void *>(get(id));
    }
    bool eraseRaw(Id id) override
    {
        return erase(id);
    }
    ItemType type() const override
    {
        return ItemTypeOf<T>;
    }
    bool emplaceRaw(Id id, HItemCreatorToken tok) override
    {
        return emplace(id, tok);
    }

    // --- NEW: CursorRaw implementation for ChunkItem<T> ---
    class Cursor final : public ChunkInterface::CursorRaw
    {
    public:
        using MapIter = typename std::unordered_map<Id, Location>::iterator;
        Cursor(ChunkItem *owner)
            : it_(owner->m_mapIndex.begin()), end_(owner->m_mapIndex.end()), owner_(owner) {}

        bool next(Id &id_out, void *&ptr_out) override
        {
            if (it_ == end_)
                return false;
            id_out = it_->first;
            const Location &loc = it_->second;
            ptr_out = static_cast<void *>(owner_->m_vecChunk[loc.chunk].buf.get() + loc.offset);
            ++it_;
            return true;
        }

    private:
        MapIter it_, end_;
        ChunkItem *owner_;
    };

    std::unique_ptr<ChunkInterface::CursorRaw> newCursor() override
    {
        return std::make_unique<Cursor>(this);
    }

    void clear() override
    {
        for (auto &kv : m_mapIndex)
        {
            T *obj = get(kv.first);
            obj->~T();
        }
        m_mapIndex.clear();
        m_vecChunk.clear();
        m_vecFreeLocation.clear();
        m_iCount = 0;
        if (!m_bLazyFirstChunk)
            allocateNewChunk();
    }

    std::size_t bytesUsed() const override
    {
        std::size_t total = 0;
        for (auto const &c : m_vecChunk)
            total += c.used;
        return total;
    }

    std::size_t bytesActuallyUsed() const { return m_iCount * sizeof(T); }

    std::size_t count() const override { return m_iCount; }

private:
    void allocateNewChunk() { m_vecChunk.emplace_back(m_iChunkBytes); }

    std::size_t m_iChunkBytes;
    bool m_bLazyFirstChunk = true;
    std::vector<Chunk> m_vecChunk;
    std::unordered_map<Id, Location> m_mapIndex;
    std::vector<Location> m_vecFreeLocation;
    std::size_t m_iCount = 0;
};