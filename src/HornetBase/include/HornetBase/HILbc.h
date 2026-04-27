#pragma once
#include "HornetBaseExport.h"
#include "HItem.h"
#include "TransactionHelper.h"
#include "HItemManager.h"
#include <span>

enum class LbcType : std::uint32_t
{
    LbcTypeUnkown,
    LbcTypeForce,
    LbcTypeConstraint,
};

class HORNETBASE_EXPORT HILbc : public HItem
{
#if !defined(_MSC_VER)
    using Super = HItem; // required on non-MSVC
#endif
    DECLARE_ITEM_ABSTRACT_TAGS(HILbc, CategoryType::CatLbc, ItemType::ItemLbc)

public:
    HILbc(Id id, HCursor *cursor, HItemCreatorToken tok);

    DEFINE_TRANSACTION_EXCHANGE(&HILbc::m_vecTarget)
public:
    virtual LbcType lbcType() const noexcept = 0;

    // Connectivity access (view into child's fixed array)
    std::span<HCursor*> targets() noexcept { return m_vecTarget; }
    std::span<HCursor* const> targets() const noexcept { return m_vecTarget; }

    void setTargets(std::span<HCursor*> cursors) noexcept
    {
        m_vecTarget.assign(cursors.begin(), cursors.end());
    }

    void addTarget(HCursor* cursor) noexcept
    {
        m_vecTarget.push_back(cursor);
    }

private:
    std::vector<HCursor*> m_vecTarget; // target connectivity (variable size, but fixed per derived type)
};