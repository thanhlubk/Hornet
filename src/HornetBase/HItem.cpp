#include "HornetBase/HItem.h"
#include <cassert>
#include <stdexcept>

HCursor::HCursor()
{
}

Id HCursor::id() const noexcept 
{ 
    return iId; 
}

CategoryType HCursor::category() const noexcept 
{
    if (m_pItem)
    {
        return m_pItem->category();
    }

    return CategoryType::CatUnknown;
}

ItemType HCursor::type() const noexcept
{
    if (m_pItem)
    {
        return m_pItem->type();
    }

    return ItemType::ItemUnkown;
}

const HItem *HCursor::itemBase() const noexcept
{
    return m_pItem;
}

HItem::HItem(Id id, HCursor *cursor, HItemCreatorToken) noexcept
    : m_pCursor(cursor)
{
    if (!m_pCursor)
    {
        assert(m_pCursor);
        std::terminate(); // DO NOT throw from a noexcept function
    }
    
    m_pCursor->iId = id;
    m_pCursor->m_pItem = this;

    // m_pCursor->stAssoc.eCategory = cat;
    // m_pCursor->stAssoc.eType = k;
}

// CategoryType HItem::category() const noexcept
// {
//     return m_pCursor->category();
// }

// ItemType HItem::type() const noexcept 
// {
//     return m_pCursor->type();
// }

Id HItem::id() const noexcept 
{
    return m_pCursor->id();
}

HCursor *HItem::getCursor() const noexcept
{
    return m_pCursor;
}