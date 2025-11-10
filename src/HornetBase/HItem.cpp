#include "HornetBase/HItem.h"
#include <cassert>
#include <stdexcept>

HCursor::HCursor() noexcept
    : stAssoc{}, iId(0), m_pItem(nullptr)
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

uint16_t HCursor::variant() const noexcept
{
    return m_pItem ? m_pItem->variant() : 0;
}

const HItem *HCursor::itemBase() const noexcept
{
    return m_pItem;
}

uint16_t HCursor::flags() const noexcept 
{ 
    return stAssoc.iFlag; 
}

void HCursor::setFlags(uint16_t v) noexcept 
{ 
    stAssoc.iFlag = v; 
}

void HCursor::setFlag(uint16_t mask) noexcept 
{ 
    stAssoc.iFlag |= mask; 
}

void HCursor::removeFlag(uint16_t mask) noexcept 
{ 
    stAssoc.iFlag &= ~mask; 
}

void HCursor::clearFlag() noexcept 
{ 
    stAssoc.iFlag = 0; 
}

uint16_t HCursor::status() const noexcept 
{ 
    return stAssoc.iStatus; 
}

void HCursor::setStatus(uint16_t v) noexcept 
{ 
    stAssoc.iStatus = v; 
}

uint16_t HCursor::renderState() const noexcept 
{ 
    return stAssoc.iRenderState; 
}

void HCursor::setRenderState(uint16_t v) noexcept 
{ 
    stAssoc.iRenderState = v; 
}

int16_t HCursor::reverse() const noexcept 
{ 
    return stAssoc.iReverse; 
}

void HCursor::setReverse(int16_t v) noexcept 
{ 
    stAssoc.iReverse = v; 
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
}

Id HItem::id() const noexcept 
{
    return m_pCursor->id();
}

HCursor *HItem::getCursor() const noexcept
{
    return m_pCursor;
}