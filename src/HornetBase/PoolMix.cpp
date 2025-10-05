#include "HornetBase/PoolMix.h"
#include "HornetBase/HItemManager.h"
#include <algorithm>
#include <stdexcept>

PoolMix::PoolMix(CategoryType cat, std::size_t chunk_bytes, bool lazy_first_chunk)
    : Pool(cat), chunk_bytes_(chunk_bytes), lazy_first_chunk_(lazy_first_chunk)
{
    if (!lazy_first_chunk_)
        new_chunk();
}

PoolMix::PoolMix(std::size_t chunk_bytes, bool lazy_first_chunk)
    : PoolMix(CategoryType::CatUnknown, chunk_bytes, lazy_first_chunk)
{
}

// ---- Pool overrides ----
void PoolMix::clear()
{
    for (auto &kv : index_)
    {
        const MixLoc &loc = kv.second;
        void *ptr = static_cast<void *>(chunks_[loc.chunk].buf.get() + loc.offset);
        loc.ops->destroy(ptr);
    }
    index_.clear();
    chunks_.clear();
    count_ = 0;
    if (!lazy_first_chunk_)
        new_chunk();
}

std::size_t PoolMix::bytesUsed() const
{
    std::size_t t = 0;
    for (const auto &c : chunks_)
        t += c.used;
    return t;
}

std::size_t PoolMix::count() const
{
    return count_;
}

void PoolMix::restoreBytes(TransactionManager::TransactionOperation tx)
{
    switch (tx.type)
    {
    case TransactionManager::TransactionType::Emplace:
        (void)eraseRaw(tx.itemType, tx.id);
        break;
    case TransactionManager::TransactionType::Erase:
        // deferred deletion: nothing to undo
        break;
    case TransactionManager::TransactionType::Modify:
        if (void *p = getRaw(tx.itemType, tx.id))
        {
            HItemManager::getInstance().restoreTransaction(tx.itemType, p, tx.payloadBefore);
        }
        break;
    }
}

void PoolMix::updateBytes(TransactionManager::TransactionOperation tx)
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

void *PoolMix::getRaw(ItemType ti, Id id)
{
    Key key{ti, id};
    auto it = index_.find(key);
    if (it == index_.end())
        return nullptr;
    const MixLoc &loc = it->second;
    return static_cast<void *>(chunks_[loc.chunk].buf.get() + loc.offset);
}

const void *PoolMix::getRawConst(ItemType ti, Id id) const
{
    Key key{ti, id};
    auto it = index_.find(key);
    if (it == index_.end())
        return nullptr;
    const MixLoc &loc = it->second;
    return static_cast<const void *>(chunks_[loc.chunk].buf.get() + loc.offset);
}

bool PoolMix::eraseRaw(ItemType ti, Id id)
{
    Key key{ti, id};
    auto it = index_.find(key);
    if (it == index_.end())
        return false;
    MixLoc loc = it->second;
    MixChunk &c = chunks_[loc.chunk];
    void *ptr = static_cast<void *>(c.buf.get() + loc.offset);
    loc.ops->destroy(ptr);

    index_.erase(it);
    --count_;

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

bool PoolMix::emplaceRaw(ItemType ti, Id id, HItemCreatorToken tok)
{
    Key key{ti, id};
    if (index_.find(key) != index_.end())
        return false;

    if (chunks_.empty())
        new_chunk();

    const HItemManager::ItemTypeDescriptor *ops = HItemManager::getInstance().descriptor(ti);
    if (!ops)
        throw std::runtime_error("Unknown ItemType in emplaceRaw");

    MixChunk *c = &chunks_.back();
    std::size_t aligned = alignUp(c->used, ops->align);

    if (aligned + ops->size > c->capacity)
    {
        // 1) reclaim holes globally
        compact_all();
        // 2) re-evaluate
        if (chunks_.empty())
            new_chunk();
        c = &chunks_.back();
        aligned = alignUp(c->used, ops->align);
        // 3) still no room → new chunk
        if (aligned + ops->size > c->capacity)
        {
            new_chunk();
            c = &chunks_.back();
            aligned = alignUp(c->used, ops->align);
        }
    }

    void *dst = static_cast<void *>(c->buf.get() + aligned);
    ops->construct(dst, id, tok);

    c->used = aligned + ops->size;
    c->ids.push_back(key);
    index_[key] = MixLoc{static_cast<std::uint32_t>(chunks_.size() - 1),
                         static_cast<std::uint32_t>(aligned), ops};
    ++count_;
    return true;
}

// ---- private helpers ----
void PoolMix::new_chunk()
{
    chunks_.emplace_back(chunk_bytes_);
}

void PoolMix::compact_all()
{
    if (index_.empty())
    {
        chunks_.clear();
        if (!lazy_first_chunk_)
            new_chunk();
        return;
    }

    struct Item
    {
        Key key;
        MixLoc loc;
    };
    std::vector<Item> items;
    items.reserve(index_.size());
    for (const auto &kv : index_)
        items.push_back({kv.first, kv.second});
    std::sort(items.begin(), items.end(), [](const Item &a, const Item &b)
              {
        if (a.loc.chunk != b.loc.chunk) return a.loc.chunk < b.loc.chunk;
        return a.loc.offset < b.loc.offset; });

    std::vector<MixChunk> new_chunks;
    new_chunks.emplace_back(chunk_bytes_);

    auto place = [&](const Item &it) -> bool
    {
        const auto *ops = it.loc.ops;
        MixChunk &nc = new_chunks.back();
        std::size_t aligned = alignUp(nc.used, ops->align);
        if (aligned + ops->size > nc.capacity)
        {
            new_chunks.emplace_back(chunk_bytes_);
            return false; // caller retries once
        }
        void *dst = static_cast<void *>(new_chunks.back().buf.get() + aligned);
        void *src = static_cast<void *>(chunks_[it.loc.chunk].buf.get() + it.loc.offset);
        ops->move(dst, src);
        ops->destroy(src);
        new_chunks.back().used = aligned + ops->size;
        new_chunks.back().ids.push_back(it.key);
        index_[it.key] = MixLoc{static_cast<std::uint32_t>(new_chunks.size() - 1),
                                static_cast<std::uint32_t>(aligned), ops};
        return true;
    };

    for (const auto &it : items)
    {
        if (!place(it))
            (void)place(it);
    }

    chunks_.swap(new_chunks);
}
