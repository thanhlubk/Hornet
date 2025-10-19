#pragma once
#include "HBaseDef.h"
#include <string>
#include <string_view>
#include <array>
#include <vector>
#include "HItem.h"
// #include "DatabaseSession.h"

class DatabaseSession;

using CaptureFn = std::string (*)(const void *, DatabaseSession *pDb);              // obj -> bytes
using RestoreFn = void (*)(void *, const std::string &bytes, DatabaseSession *pDb); // bytes -> obj
using DestroyFn = void (*)(void *);
using MoveFn = void (*)(void *dst, void *src);
using ConstructFn = void (*)(void *dst, Id id, HCursor *cursor, HItemCreatorToken);

struct ItemTypeDescriptor
{
    CategoryType cat{};
    ItemType kind{ItemType::ItemEnd};
    std::size_t size{0};
    std::size_t align{0};
    DestroyFn destroy{};
    MoveFn move{};
    ConstructFn construct{};
    CaptureFn capture{};
    RestoreFn restore{};
};

// Singleton class to hold the ItemType to CategoryType mapping
class HORNETBASE_EXPORT HItemManager
{
public:
    // Get the singleton instance
    static HItemManager &getInstance() noexcept;

    // Get CategoryType for a given ItemType
    CategoryType getCategory(ItemType kind) const noexcept;

    // Delete copy and move operations
    HItemManager(const HItemManager &) = delete;
    HItemManager &operator=(const HItemManager &) = delete;
    HItemManager(HItemManager &&) = delete;
    HItemManager &operator=(HItemManager &&) = delete;

    bool registerInfor(CategoryType cat, ItemType kind);

    // capture to bytes (no-op if unregistered)
    std::string captureTransaction(ItemType k, const void *obj, DatabaseSession *pDb) const;
    // restore from bytes (no-op if unregistered)
    void restoreTransaction(ItemType k, void *obj, const std::string &bytes, DatabaseSession *pDb) const;

    template <class T>
    bool registerTransaction()
    {
        const auto idx = static_cast<std::size_t>(ItemTypeOf<T>);
        m_arrCapture[idx] = [](const void *p, DatabaseSession *pDb) -> std::string
        {
            const T &self = *reinterpret_cast<const T *>(p);
            return T::captureTransactionItem(self, pDb);
        };
        m_arrRestore[idx] = [](void *p, const std::string &bytes, DatabaseSession *pDb)
        {
            T &self = *reinterpret_cast<T *>(p);
            T::restoreTransactionItem(self, bytes, pDb);
        };
        return true;
    }

    // using CaptureFn = std::string (*)(const void *, DatabaseSession *pDb);              // obj -> bytes
    // using RestoreFn = void (*)(void *, const std::string &bytes, DatabaseSession *pDb); // bytes -> obj
    // using DestroyFn = void (*)(void *);
    // using MoveFn = void (*)(void *dst, void *src);
    // using ConstructFn = void (*)(void *dst, Id id, HCursor *cursor, HItemCreatorToken);

    // struct ItemTypeDescriptor
    // {
    //     CategoryType cat{};
    //     ItemType kind{ItemType::ItemEnd};
    //     std::size_t size{0};
    //     std::size_t align{0};
    //     DestroyFn destroy{};
    //     MoveFn move{};
    //     ConstructFn construct{};
    //     CaptureFn capture{};
    //     RestoreFn restore{};
    // };

    template <class T>
    bool registerType(CategoryType cat, ItemType kind)
    {
        const auto idx = static_cast<std::size_t>(kind);
        // codecs (you already have)
        registerTransaction<T>();

        // fill ops
        m_arrTypeDesc[idx] = ItemTypeDescriptor{
            cat, kind,
            sizeof(T), alignof(T),
            [](void *p)
            { reinterpret_cast<T *>(p)->~T(); },
            [](void *dst, void *src)
            {
                if constexpr (std::is_move_constructible_v<T>)
                    ::new (dst) T(std::move(*reinterpret_cast<T *>(src)));
                else
                    ::new (dst) T(*reinterpret_cast<T *>(src));
            },
            [](void *dst, Id id, HCursor *cursor, HItemCreatorToken tok)
            { ::new (dst) T(id, cursor, tok); },
            m_arrCapture[idx], m_arrRestore[idx]};
        // keep your category map as-is
        registerInfor(cat, kind);
        return true;
    }

    const ItemTypeDescriptor *descriptor(ItemType k) const;

    std::vector<ItemType> getRelatedItemType(CategoryType cat, bool only_registered = false) const;

private:
    // Private constructor to prevent instantiation
    HItemManager();

    std::array<CategoryType, static_cast<uint16_t>(ItemType::ItemEnd)> m_arrItemToCategory{CategoryType::CatUnknown};

    // ---- NEW: function tables ----
    std::array<CaptureFn, static_cast<uint16_t>(ItemType::ItemEnd)> m_arrCapture{};
    std::array<RestoreFn, static_cast<uint16_t>(ItemType::ItemEnd)> m_arrRestore{};

    std::array<std::optional<ItemTypeDescriptor>, static_cast<uint16_t>(ItemType::ItemEnd)> m_arrTypeDesc
    {};
};