#include "HornetBase/HItemManager.h"
#include <optional>
#include <array>
#include <unordered_map>

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

std::string HItemManager::captureTransaction(ItemTypeVariant k, const void *obj, DatabaseSession *pDb) const
{
    auto idx = std::hash<ItemTypeVariant>{}(k);
    if (auto it = m_mapCapture.find(idx); it != m_mapCapture.end() && it->second)
    {
        return it->second(obj, pDb);
    }
    // auto idx = static_cast<std::size_t>(k); 
    // if (idx < m_mapCapture.size() && m_mapCapture[idx])
    // {
    //     return m_mapCapture[idx](obj, pDb);
    // }
    return {};
}

void HItemManager::restoreTransaction(ItemTypeVariant k, void *obj, const std::string &bytes, DatabaseSession *pDb) const
{
    auto idx = std::hash<ItemTypeVariant>{}(k);
    if (auto it = m_mapRestore.find(idx); it != m_mapRestore.end() && it->second)
    {
        it->second(obj, bytes, pDb);
    }
    // auto idx = static_cast<std::size_t>(k);
    // if (idx < m_mapRestore.size() && m_mapRestore[idx])
    // {
    //     m_mapRestore[idx](obj, bytes, pDb);
    // }
}

const ItemTypeDescriptor *HItemManager::descriptor(ItemTypeVariant k) const
{
    auto idx = std::hash<ItemTypeVariant>{}(k);
    if (auto it = m_mapTypeDesc.find(idx); it != m_mapTypeDesc.end() && it->second.has_value())
    {
        return &it->second.value(); // address of value inside the node
    }
    // auto idx = static_cast<std::size_t>(k);
    // if (idx < m_mapTypeDesc.size() && m_mapTypeDesc[idx].has_value())
    // {
    //     return &*m_mapTypeDesc[idx];
    // }
    return nullptr;
}

std::vector<ItemType>
HItemManager::getRelatedItemType(CategoryType cat, bool only_registered) const
{
    std::vector<ItemType> out;
    const auto N = static_cast<std::uint16_t>(ItemType::ItemEnd); // enum range
    out.reserve(8);                                               // small guess

    // for (std::uint16_t i = 0; i < N; ++i)
    // {
    //     const auto k = static_cast<ItemType>(i);
    //     if (m_arrItemToCategory[i] != cat)
    //         continue; // ItemType -> CategoryType map
    //     if (only_registered)
    //     {
    //         if (!(i < m_mapTypeDesc.size() && m_mapTypeDesc[i].has_value())) // has ItemTypeDescriptor?
    //             continue;
    //     }
    //     out.emplace_back(k);
    // }

    for (auto itr = m_mapTypeDesc.begin(); itr != m_mapTypeDesc.end(); itr++)
    {
        if (itr->second.has_value())
        {
            if (itr->second.value().cat == cat)
                out.emplace_back(itr->second.value().kind);
        }
    }
    
    return out;
}