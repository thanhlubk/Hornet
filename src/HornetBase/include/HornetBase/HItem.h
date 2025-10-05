#pragma once
#include "HornetBaseExport.h"
#include "HBaseDef.h"
#include <cstddef>
#include <concepts>
#include <optional>
#include <type_traits>

#pragma pack(push, 2)
struct AssociateParam
{
    CategoryType eCategory;
    ItemType eType;
    uint16_t iFlag;
    uint16_t iStatus;
};
#pragma pack(pop)

class HORNETBASE_EXPORT HCursor
{
    friend class HItem;

protected:
    HCursor(Id id, CategoryType cat, ItemType type);
    HCursor(const HCursor &) = delete;
    HCursor &operator=(const HCursor &) = delete;
    HCursor(HCursor &&) noexcept = default;
    HCursor &operator=(HCursor &&) noexcept = default;
    virtual ~HCursor() = default;

public:
    Id id() const noexcept;
    CategoryType category() const noexcept;
    ItemType type() const noexcept;

private:
    AssociateParam stAssoc;
    Id iId;
};

struct HItemCreatorToken
{
private:
    HItemCreatorToken() = default;
    friend class DatabaseSession;
};

class HORNETBASE_EXPORT HItem
{
public:
    CategoryType category() const noexcept;
    ItemType type() const noexcept;
    Id id() const noexcept;
    HCursor *getCursor() noexcept;

    virtual ~HItem() = default;

protected:
    HItem(Id id, HItemCreatorToken, CategoryType cat, ItemType k) noexcept;
    HItem(const HItem &) = delete;
    HItem &operator=(const HItem &) = delete;
    HItem(HItem &&) noexcept = default;
    HItem &operator=(HItem &&) noexcept = default;

private: 
    HCursor m_stCursor;
};

template <class T>
concept HItemTemplate = std::is_base_of_v<HItem, T>;

template <HItemTemplate T>
inline constexpr CategoryType CategoryTypeOf = T::CategoryTypeTag;

template <HItemTemplate T>
inline constexpr ItemType ItemTypeOf = T::ItemTypeTag;

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
    static constexpr CategoryType category_static() noexcept { return CategoryTypeTag; } \
    static constexpr ItemType kind_static() noexcept { return ItemTypeTag; }