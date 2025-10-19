#include "HornetBase/HItemManager.h"
#include <optional>
#include <array>

HItemManager &HItemManager::getInstance() noexcept
{
    static HItemManager instance;
    return instance;
}

HItemManager::HItemManager() = default;

CategoryType HItemManager::getCategory(ItemType kind) const noexcept
{
    return m_arrItemToCategory[static_cast<std::size_t>(kind)];
}

bool HItemManager::registerInfor(CategoryType cat, ItemType kind)
{
    if (static_cast<uint16_t>(cat) >= static_cast<uint16_t>(CategoryType::CatEnd) ||
        static_cast<uint16_t>(kind) >= static_cast<uint16_t>(ItemType::ItemEnd))
    {
        return false;
    }
    auto idx = static_cast<std::size_t>(kind);
    if (m_arrItemToCategory[idx] == CategoryType::CatUnknown)
    {
        m_arrItemToCategory[idx] = cat;
        return true;
    }
    return false;
}

std::string HItemManager::captureTransaction(ItemType k, const void *obj, DatabaseSession *pDb) const
{
    auto idx = static_cast<std::size_t>(k);
    if (idx < m_arrCapture.size() && m_arrCapture[idx])
    {
        return m_arrCapture[idx](obj, pDb);
    }
    return {};
}

void HItemManager::restoreTransaction(ItemType k, void *obj, const std::string &bytes, DatabaseSession *pDb) const
{
    auto idx = static_cast<std::size_t>(k);
    if (idx < m_arrRestore.size() && m_arrRestore[idx])
    {
        m_arrRestore[idx](obj, bytes, pDb);
    }
}

const ItemTypeDescriptor *HItemManager::descriptor(ItemType k) const
{
    auto idx = static_cast<std::size_t>(k);
    if (idx < m_arrTypeDesc.size() && m_arrTypeDesc[idx].has_value())
    {
        return &*m_arrTypeDesc[idx];
    }
    return nullptr;
}

std::vector<ItemType>
HItemManager::getRelatedItemType(CategoryType cat, bool only_registered) const
{
    std::vector<ItemType> out;
    const auto N = static_cast<std::uint16_t>(ItemType::ItemEnd); // enum range
    out.reserve(8);                                               // small guess

    for (std::uint16_t i = 0; i < N; ++i)
    {
        const auto k = static_cast<ItemType>(i);
        if (m_arrItemToCategory[i] != cat)
            continue; // ItemType -> CategoryType map
        if (only_registered)
        {
            if (!(i < m_arrTypeDesc.size() && m_arrTypeDesc[i].has_value())) // has ItemTypeDescriptor?
                continue;
        }
        out.emplace_back(k);
    }
    return out;
}