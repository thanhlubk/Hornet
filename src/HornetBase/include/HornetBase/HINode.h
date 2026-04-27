#pragma once
#include "HornetBaseExport.h"
#include "HItem.h"
#include "TransactionHelper.h"
#include "HItemManager.h"
#include "HornetUtil/HVector.h"
#include "HornetUtil/HColor.h"

enum class NodeTypeExtended : std::uint8_t
{
    NodeTypeExtendedStandard,
    NodeTypeExtendedHeaviside,
    NodeTypeExtendedAsymptotic,
};

class HORNETBASE_EXPORT HINode : public HItem
{
#if !defined(_MSC_VER)
    using Super = HItem; // required on non-MSVC
#endif
    DECLARE_ITEM_TAGS(HINode, CategoryType::CatNode, ItemType::ItemNode)

public:
    HINode(Id id, HCursor *cursor, HItemCreatorToken tok);

    DEFINE_TRANSACTION_EXCHANGE(&HINode::m_vPosition,
                                &HINode::m_color,
                                &HINode::m_eTypeExtended)
public:
    // const std::array<uint64_t, 5> &values() const noexcept;
    // std::array<uint64_t, 5> &values() noexcept;

    NodeTypeExtended nodeTypeExtended() const noexcept { return m_eTypeExtended; }
    void setNodeTypeExtended(NodeTypeExtended t) noexcept { m_eTypeExtended = t; }

    HVector3d position() const noexcept { return m_vPosition; }
    void setPosition(const HVector3d &pos) noexcept { m_vPosition = pos; }

    HColor color() const noexcept { return m_color; }
    void setColor(const HColor &color) noexcept { m_color = color; }

private:
    HVector3d m_vPosition; // position
    HColor m_color;    // color
    NodeTypeExtended m_eTypeExtended; // node type extended (optional; default standard)
};

class HORNETBASE_EXPORT HINodeHeat : public HINode
{
#if !defined(_MSC_VER)
    using Super = HINode; // required on non-MSVC
#endif
    DECLARE_ITEM_TAGS(HINodeHeat, CategoryType::CatNode, ItemType::ItemNodeHeat)

public:
    HINodeHeat(Id id, HCursor *cursor, HItemCreatorToken tok);

    DEFINE_TRANSACTION_EXCHANGE(&HINodeHeat::m_temperature)
public:
    // const std::array<uint64_t, 5> &values() const noexcept;
    // std::array<uint64_t, 5> &values() noexcept;

private:
    double m_temperature; // temperature
};