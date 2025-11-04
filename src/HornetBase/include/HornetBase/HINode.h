#pragma once
#include "HornetBaseExport.h"
#include "HItem.h"
#include "TransactionHelper.h"
#include "HItemManager.h"
#include "HornetUtil/HVector.h"
#include "HornetUtil/HColor.h"

class HORNETBASE_EXPORT HINode : public HItem
{
#if !defined(_MSC_VER)
    using Super = HItem; // required on non-MSVC
#endif
    DECLARE_ITEM_STATIC_TAGS(HINode, CategoryType::CatNode, ItemType::ItemNode)

public:
    HINode(Id id, HCursor *cursor, HItemCreatorToken tok);

    DEFINE_TRANSACTION_EXCHANGE(&HINode::m_vPosition,
                                &HINode::m_color)
public:
    // const std::array<uint64_t, 5> &values() const noexcept;
    // std::array<uint64_t, 5> &values() noexcept;

private:
    HVector3d m_vPosition; // position
    HColor m_color;    // color
};

class HORNETBASE_EXPORT HINodeHeat : public HINode
{
#if !defined(_MSC_VER)
    using Super = HINode; // required on non-MSVC
#endif
    DECLARE_ITEM_STATIC_TAGS(HINodeHeat, CategoryType::CatNode, ItemType::ItemNodeHeat)

public:
    HINodeHeat(Id id, HCursor *cursor, HItemCreatorToken tok);

    DEFINE_TRANSACTION_EXCHANGE(&HINodeHeat::m_temperature)
public:
    // const std::array<uint64_t, 5> &values() const noexcept;
    // std::array<uint64_t, 5> &values() noexcept;

private:
    double m_temperature; // temperature
};