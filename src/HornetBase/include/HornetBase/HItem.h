#pragma once
#include "HornetBaseExport.h"
#include "HBaseDef.h"
#include <cstddef>
#include <concepts>
#include <optional>
#include <type_traits>
#include <string>

#pragma pack(push, 2)
struct AssociateParam
{
    // CategoryType eCategory;
    // ItemType eType;
    uint16_t iFlag;
    uint16_t iStatus;

    AssociateParam()
        // : eCategory(CategoryType::CatUnknown),
        //   eType(ItemType::ItemUnkown),
        : iFlag(0),
          iStatus(0) {}
};
#pragma pack(pop)

class HCursor;

struct HItemCreatorToken
{
private:
    HItemCreatorToken() = default;
    friend class DatabaseSession;
};

class HORNETBASE_EXPORT HItem
{
public:
    virtual CategoryType category() const noexcept = 0;
    virtual ItemType type() const noexcept = 0;
    Id id() const noexcept;
    HCursor *getCursor() const noexcept;

    virtual ~HItem() = default;

    virtual std::string captureTransactionItem(DatabaseSession *db) const { return std::string(); }
    virtual size_t restoreTransactionItem(const std::string &bytes, size_t off, DatabaseSession *db) { return 0; }

protected:
    HItem(Id id, HCursor *cursor, HItemCreatorToken) noexcept;
    HItem(const HItem &) = delete;
    HItem &operator=(const HItem &) = delete;
    HItem(HItem &&) noexcept = default;
    HItem &operator=(HItem &&) noexcept = default;

private: 
    HCursor* m_pCursor;
};

template <class T>
concept HItemTemplate = std::is_base_of_v<HItem, T>;

template <HItemTemplate T>
inline constexpr CategoryType CategoryTypeOf = T::CategoryTypeTag;

template <HItemTemplate T>
inline constexpr ItemType ItemTypeOf = T::ItemTypeTag;

class HORNETBASE_EXPORT HCursor
{
    friend class HItem;
    friend class DatabaseSession;
    friend class Pool;
    friend class PoolMix;
    friend class PoolUnique;
    template <class T>
    friend class ChunkItem;
    friend struct Chunk;

public:
    HCursor();
    virtual ~HCursor() = default;

protected:
    HCursor(const HCursor &) = delete;
    HCursor &operator=(const HCursor &) = delete;
    HCursor(HCursor &&) noexcept = default;
    HCursor &operator=(HCursor &&) noexcept = default;

public:
    Id id() const noexcept;
    CategoryType category() const noexcept;
    ItemType type() const noexcept;

    // Return the owning base pointer (read-only view)
    const HItem *itemBase() const noexcept;

    // Typed read-only downcast; returns nullptr on mismatch
    template <HItemTemplate T>
    inline const T *item() const noexcept
    {
        if (!m_pItem)
            return nullptr;
        if (m_pItem->category() != CategoryTypeOf<T>)
            return nullptr;
        if (m_pItem->type() != ItemTypeOf<T>)
            return nullptr;
        return static_cast<const T *>(m_pItem);
    }

    // Optional: quick type check
    template <HItemTemplate T>
    inline bool isType() const noexcept
    {
        return m_pItem->category() == CategoryTypeOf<T> && m_pItem->type() == ItemTypeOf<T>;
    }

private:
    AssociateParam stAssoc;
    Id iId;
    HItem *m_pItem = nullptr; // back-pointer to owning HItem (internal only)
};

// (type,id) key so same numeric Id can exist per type
struct Key
{
    ItemType type;
    Id id;
    bool operator==(const Key &o) const noexcept 
    {
        return type == o.type && id == o.id;
    }
};

struct KeyHash
{
    std::size_t operator()(const Key &key) const noexcept
    {
        // Use std::hash for integers to get the hash for each member.
        // std::hash<int>() is a standard hash function for integers.
        std::size_t hash1 = std::hash<ItemType>{}(key.type);
        std::size_t hash2 = std::hash<Id>{}(key.id);

        // Combine the two hashes in a robust way to create a single hash value.
        // A common and effective method is to use a bit shift and XOR operation.
        // The magic number 0x9e3779b97f4a7c15ULL is a recommended constant for this.
        return hash1 ^ (hash2 + 0x9e3779b97f4a7c15ULL + (hash1 << 6) + (hash1 >> 2));
    }
};

#define DECLARE_ITEM_STATIC_TAGS(className, cat, kind)                                   \
    static inline const bool kTypeDescRegistered =                                       \
        HItemManager::getInstance().registerType<className>(cat, kind);                  \
                                                                                         \
public:                                                                                  \
    static constexpr CategoryType CategoryTypeTag = (cat);                               \
    static constexpr ItemType ItemTypeTag = (kind);                                      \
    static constexpr CategoryType categoryTag() noexcept { return CategoryTypeTag; } \
    static constexpr ItemType typeTag() noexcept { return ItemTypeTag; } \
    virtual CategoryType category() const noexcept override { return cat; } \
    virtual ItemType type() const noexcept override { return kind; } \
