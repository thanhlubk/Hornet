#pragma once
#include "HornetBase/HItem.h"

HCursor::HCursor(Id id, CategoryType cat, ItemType type)
    : stAssoc{cat, type, 0, 0}, iId(id)
{
}

Id HCursor::id() const noexcept 
{ 
    return iId; 
}

CategoryType HCursor::category() const noexcept 
{ 
    return stAssoc.eCategory; 
}

ItemType HCursor::type() const noexcept 
{ 
    return stAssoc.eType; 
}

HItem::HItem(Id id, HItemCreatorToken, CategoryType cat, ItemType k) noexcept
    : m_stCursor(id, cat, k)
{
}

CategoryType HItem::category() const noexcept 
{ 
    return m_stCursor.category(); 
}

ItemType HItem::type() const noexcept 
{ 
    return m_stCursor.type(); 
}

Id HItem::id() const noexcept 
{ 
    return m_stCursor.id(); 
}

HCursor *HItem::getCursor() noexcept 
{ 
    return &m_stCursor; 
}