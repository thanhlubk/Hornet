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
#include <vector>
#include <algorithm>
#include <cassert>

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
// Non-templated arena for HCursor (no index, no get, hole reuse, no compaction)
// --------------------------------------------------------------------------------------
class ChunkCursor
{
public:
    explicit ChunkCursor(std::size_t chunk_bytes = kDefaultChunkBytes,
                         bool lazy_first_chunk = true)
        : m_chunkBytes(chunk_bytes), m_lazy(lazy_first_chunk)
    {
        // ensure chunk size is aligned to HCursor alignment boundary
        const std::size_t a = alignof(HCursor);
        if (m_chunkBytes & (a - 1))
            m_chunkBytes = alignUp(m_chunkBytes, a);
        if (!m_lazy)
            allocateNewChunk();
    }

    ChunkCursor(const ChunkCursor &) = delete;
    ChunkCursor &operator=(const ChunkCursor &) = delete;
    ChunkCursor(ChunkCursor &&) = default;
    ChunkCursor &operator=(ChunkCursor &&) = default;

    // Create a new HCursor in-place and return its pointer.
    // (Fields are initialized later by the HItem that owns it.)
    HCursor *emplace()
    {
        // lazy first chunk
        if (m_chunks.empty())
            allocateNewChunk();

        // reuse hole if available (LIFO)
        if (!m_free.empty())
        {
            const Location loc = m_free.back();
            m_free.pop_back();
            void *p = static_cast<void *>(m_chunks[loc.chunk].buf.get() + loc.offset);
            HCursor *cur = ::new (p) HCursor();
            ++m_count;
            return cur;
        }

        // append to current chunk (with alignment)
        Chunk *c = &m_chunks.back();
        const std::size_t a = alignof(HCursor);
        const std::size_t s = sizeof(HCursor);
        std::size_t aligned = alignUp(c->used, a);

        if (aligned + s > c->capacity)
        {
            allocateNewChunk();
            c = &m_chunks.back();
            aligned = alignUp(c->used, a);
        }

        void *p = static_cast<void *>(c->buf.get() + aligned);
        HCursor *cur = ::new (p) HCursor();
        c->used = aligned + s;
        ++m_count;
        return cur;
    }

    // Destroy the given HCursor and record a hole for reuse.
    // Pointer must have been returned by this ChunkCursor.
    void erase(HCursor *ptr)
    {
        if (!ptr)
            return;

        // find owning chunk & offset (linear in #chunks; #chunks is small)
        Location loc{};
        if (!findLocation(ptr, loc))
        {
            // pointer not owned by this arena; ignore or assert as you prefer
            // assert(false && "erase() with foreign pointer");
            return;
        }

        ptr->~HCursor();
        --m_count;
        m_free.push_back(loc);
    }

    void clear()
    {
        // call dtors on all live objects by scanning used regions and free list
        // Simpler approach: we only know hole locations; so destroy everything
        // except holes. For speed, we destroy by reading at every occupied slot.
        for (std::size_t ci = 0; ci < m_chunks.size(); ++ci)
        {
            Chunk &c = m_chunks[ci];
            // Build a quick set of hole offsets for this chunk
            // (small; number of holes << number of objects)
            std::vector<std::uint32_t> holes;
            holes.reserve(m_free.size());
            for (auto &h : m_free)
                if (h.chunk == ci)
                    holes.push_back(h.offset);
            std::sort(holes.begin(), holes.end());

            const std::size_t a = alignof(HCursor);
            const std::size_t s = sizeof(HCursor);
            for (std::size_t off = 0; off + s <= c.used;)
            {
                std::size_t aligned = alignUp(off, a);
                if (aligned + s > c.used)
                    break;

                // skip if this offset is a recorded hole
                if (std::binary_search(holes.begin(), holes.end(),
                                       static_cast<std::uint32_t>(aligned)))
                {
                    off = aligned + s;
                    continue;
                }

                void *p = static_cast<void *>(c.buf.get() + aligned);
                std::launder(reinterpret_cast<HCursor *>(p))->~HCursor();
                off = aligned + s;
            }
        }

        m_chunks.clear();
        m_free.clear();
        m_count = 0;
        if (!m_lazy)
            allocateNewChunk();
    }

    std::size_t count() const noexcept { return m_count; }

    // bytes reserved/advanced inside chunks (includes holes)
    std::size_t bytesUsed() const noexcept
    {
        std::size_t t = 0;
        for (auto const &c : m_chunks)
            t += c.used;
        return t;
    }

private:
    void allocateNewChunk() { m_chunks.emplace_back(m_chunkBytes); }

    // O(#chunks) locate owning chunk and offset for a pointer
    bool findLocation(const HCursor *p, Location &out) const
    {
        const std::byte *pp = reinterpret_cast<const std::byte *>(p);
        for (std::uint32_t i = 0; i < m_chunks.size(); ++i)
        {
            const Chunk &c = m_chunks[i];
            const std::byte *base = c.buf.get();
            if (pp >= base && pp < base + c.used)
            {
                out.chunk = i;
                out.offset = static_cast<std::uint32_t>(pp - base);
                return true;
            }
        }
        return false;
    }

private:
    std::size_t m_chunkBytes;
    bool m_lazy = true;
    std::vector<Chunk> m_chunks;
    std::vector<Location> m_free; // holes (chunk,offset), reused LIFO
    std::size_t m_count = 0;
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

    explicit ChunkItem(std::size_t chunk_bytes = kDefaultChunkBytes, bool lazy_first_chunk = true, ChunkCursor *chunkCursor = nullptr)
        : m_iChunkBytes(chunk_bytes), m_bLazyFirstChunk(lazy_first_chunk), m_pChunkCursor(chunkCursor)
    {
        if ((m_iChunkBytes & (alignof(T) - 1)) != 0)
        {
            m_iChunkBytes = alignUp(m_iChunkBytes, alignof(T));
        }
        if (!m_bLazyFirstChunk)
        {
            allocateNewChunk(); // eager allocation
        }

        // optional safety
        assert(m_pChunkCursor && "ChunkItem requires a valid ChunkCursor*");
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
            HCursor* pCursor = m_pChunkCursor->emplace();
            T *obj = ::new (ptr) T(id, pCursor, std::forward<Args>(args)...);
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
        HCursor *pCursor = m_pChunkCursor->emplace();
        T *obj = ::new (ptr) T(id, pCursor, std::forward<Args>(args)...);
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

        // Erase cursor first
        if (T *obj = get(it->first))
            m_pChunkCursor->erase(obj->getCursor());

        // Destroy object
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
            // rebind the cursor owner to the new address
            dst_obj->getCursor()->m_pItem = dst_obj;
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

    // ---- Pointer iterators/ranges (T* / const T*) ----
    class Iterator
    {
    public:
        using MapIter = typename std::unordered_map<Id, Location>::iterator;
        using difference_type = std::ptrdiff_t;
        using value_type = T *;
        using reference = T *;
        using pointer = T *;
        using iterator_category = std::forward_iterator_tag;

        Iterator(MapIter it, ChunkItem *owner) : it_(it), owner_(owner) {}

        reference operator*() const
        {
            const Location &loc = it_->second;
            return std::launder(reinterpret_cast<T *>(owner_->m_vecChunk[loc.chunk].buf.get() + loc.offset));
        }
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

    class IteratorConst
    {
    public:
        using MapIter = typename std::unordered_map<Id, Location>::const_iterator;
        using difference_type = std::ptrdiff_t;
        using value_type = const T *;
        using reference = const T *;
        using pointer = const T *;
        using iterator_category = std::forward_iterator_tag;

        IteratorConst(MapIter it, const ChunkItem *owner) : it_(it), owner_(owner) {}

        reference operator*() const
        {
            const Location &loc = it_->second;
            return std::launder(reinterpret_cast<const T *>(owner_->m_vecChunk[loc.chunk].buf.get() + loc.offset));
        }
        IteratorConst &operator++()
        {
            ++it_;
            return *this;
        }
        bool operator==(const IteratorConst &rhs) const { return it_ == rhs.it_; }
        bool operator!=(const IteratorConst &rhs) const { return it_ != rhs.it_; }

    private:
        MapIter it_;
        const ChunkItem *owner_;
    };

    struct Range
    {
        ChunkItem &p;
        Iterator begin() { return Iterator(p.m_mapIndex.begin(), &p); }
        Iterator end() { return Iterator(p.m_mapIndex.end(), &p); }
    };

    struct RangeConst
    {
        const ChunkItem &p;
        IteratorConst begin() const { return IteratorConst(p.m_mapIndex.cbegin(), &p); }
        IteratorConst end() const { return IteratorConst(p.m_mapIndex.cend(), &p); }
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
    ChunkCursor *m_pChunkCursor;
};