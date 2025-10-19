#pragma once
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
    return stAssoc.eCategory; 
}

ItemType HCursor::type() const noexcept 
{ 
    return stAssoc.eType; 
}

HItem::HItem(Id id, HCursor *cursor, HItemCreatorToken, CategoryType cat, ItemType k) noexcept
    : m_pCursor(cursor)
{
    if (!m_pCursor)
    {
        assert(m_pCursor);
        std::terminate(); // DO NOT throw from a noexcept function
    }
    
    m_pCursor->iId = id;
    m_pCursor->stAssoc.eCategory = cat;
    m_pCursor->stAssoc.eType = k;
    m_pCursor->m_pItem = this;
}

CategoryType HItem::category() const noexcept
{
    return m_pCursor->category();
}

ItemType HItem::type() const noexcept 
{
    return m_pCursor->type();
}

Id HItem::id() const noexcept 
{
    return m_pCursor->id();
}

HCursor *HItem::getCursor() const noexcept
{
    return m_pCursor;
}