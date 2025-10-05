#pragma once
#include "HBaseDef.h"
#include "PoolMix.h"
#include "PoolUnique.h"
#include "TransactionManager.h"

// -----------------------------------------------------------------------------
// DatabaseSession: holds multiple Pool stores (PoolUnique / PoolMix)
// -----------------------------------------------------------------------------

class HORNETBASE_EXPORT DatabaseSession
{
public:
    enum class PoolType
    {
        Unique,
        Mix
    };

    explicit DatabaseSession(std::size_t chunk_bytes_per_store = kDefaultChunkBytes, bool lazy_first_chunk = true);

    DatabaseSession(const DatabaseSession &) = delete;
    DatabaseSession &operator=(const DatabaseSession &) = delete;
    DatabaseSession(DatabaseSession &&) noexcept = default;
    DatabaseSession &operator=(DatabaseSession &&) noexcept = default;

    // --- Create / register stores -------------------------------------------
    // Convenience: add by kind
    void add(CategoryType cat);

    // Create on first use if absent (kind decides what to create)
    // If absent and no kind is specified, defaults to Unique.
    Pool *ensure(CategoryType cat);

    // --- Lookup store objects ------------------------------------------------
    bool contain(CategoryType cat) const;

    Pool *getPool(CategoryType cat);
    const Pool *getPool(CategoryType cat) const;

    // Strongly-typed access to concrete stores when you need their custom APIs:
    PoolUnique *getPoolUnique(CategoryType cat);
    PoolMix *getPoolMix(CategoryType cat);

    // --- Data operations (templated) -----------------------------------------
    // Emplace routes to the right derived class (PoolUnique or PoolMix).
    template <HItemTemplate T, class... Args>
    bool emplace(Id id, Args &&...args)
    {
        if (!m_transaction.isOpen())
            return false;

        // existing: route by CategoryType to Unique/Mix
        constexpr CategoryType cat = CategoryTypeOf<T>;
        PoolType kind = static_cast<PoolType>(m_mapCategoryPoolType.at(cat));

        HItemCreatorToken tok{};
        Pool *base = ensure(cat);

        bool bRet = false;
        if (kind == PoolType::Unique)
        {
            auto pBase = dynamic_cast<PoolUnique *>(base);
            bRet = pBase->template emplace<T>(id, tok, std::forward<Args>(args)...);
        }
        else
        {
            auto pBase = dynamic_cast<PoolMix *>(base);
            bRet = pBase->template emplace<T>(id, tok, std::forward<Args>(args)...);
        }
        m_transaction.template noteEmplace<T>(id);

        return bRet;
    }

    template <HItemTemplate T>
    T *checkOut(Id id)
    {
        if (!m_transaction.isOpen())
            return nullptr;

        if (m_transaction.template isNoteErase<T>(id))
            return nullptr;

        constexpr CategoryType cat = CategoryTypeOf<T>;
        if (auto *base = getPool(cat))
        {
            T *p = base->template get<T>(id);
            if (p)
            {
                std::string payloadBefore;
                if (auto *store = getPool(HItemManager::getInstance().getCategory(ItemTypeOf<T>)))
                {
                    if (auto const *p = store->getRawConst(ItemTypeOf<T>, id))
                    {
                        payloadBefore = HItemManager::getInstance().captureTransaction(ItemTypeOf<T>, p);
                    }
                }
                m_transaction.template noteCheckOut<T>(id, payloadBefore);
                return p;
            }
        }
        return nullptr;
    }

    template <HItemTemplate T>
    const T *get(Id id) const
    {
        if (!std::is_base_of_v<HItem, T>)
            throw std::runtime_error("Unknown item type");

        if (m_transaction.template isNoteErase<T>(id))
            return nullptr;

        constexpr CategoryType cat = CategoryTypeOf<T>;
        if (auto *base = getPool(cat))
            return base->template get<T>(id);
        return nullptr;
    }

    template <HItemTemplate T>
    bool erase(Id id)
    {
        constexpr CategoryType cat = CategoryTypeOf<T>;
        if (m_transaction.isOpen())
        {
            std::string payloadBefore;
            if (m_transaction.template isNoteEmplace<T>(id))
            {
                if (auto *store = getPool(cat))
                    store->eraseRaw(ItemTypeOf<T>, id);
            }
            else if (!m_transaction.template isNoteModify<T>(id))
            {
                if (auto *store = getPool(cat))
                {
                    if (auto const *p = store->getRawConst(ItemTypeOf<T>, id))
                    {
                        payloadBefore = HItemManager::getInstance().captureTransaction(ItemTypeOf<T>, p);
                    }
                }
            }
            // Defer the erase until commit; keep object visible now.
            m_transaction.template noteErase<T>(id, payloadBefore);
            return true;
        }
        return false;
    }

    // --- Housekeeping --------------------------------------------------------
    void clearCategory(CategoryType cat);

    void clear();

    // --- Stats ---------------------------------------------------------------
    struct Stats
    {
        std::size_t store_count = 0;
        std::size_t bytes_used = 0;
        std::size_t objects = 0;
    };

    Stats stats() const;

    // ---- Transaction control ----
    bool isTransactionOpen() const noexcept;
    void beginTransaction();
    void commitTransaction();
    void rollbackTransaction();
    bool undo();
    bool redo();

protected:
    // Recreate an erased object and restore its BEFORE state (used by UNDO of Erase)
    bool undoUpdateBytes(const TransactionManager::TransactionOperation &op);

    // Recreate an inserted object and restore its AFTER state (used by REDO of Emplace)
    bool redoUpdateBytes(const TransactionManager::TransactionOperation &op);

private:
    struct EnumHash
    {
        using UnderType = std::underlying_type_t<CategoryType>;
        std::size_t operator()(CategoryType cat) const noexcept
        {
            return std::hash<UnderType>{}(static_cast<UnderType>(cat));
        }
    };

    std::unordered_map<CategoryType, std::unique_ptr<Pool>, EnumHash> m_mapCategoryPool;
    std::size_t m_iChunkBytes;
    bool m_bLazy;
    TransactionManager m_transaction;
    std::unordered_map<CategoryType, PoolType, EnumHash> m_mapCategoryPoolType;
};