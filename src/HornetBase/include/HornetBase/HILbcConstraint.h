#pragma once
#include "HornetBaseExport.h"
#include "HILbc.h"
#include "HornetUtil/HVector.h"

enum class LbcConstraintDof : uint32_t
{
    LbcConstraintDofNone = 0,
    LbcConstraintDofTx = 1 << 0,
    LbcConstraintDofTy = 1 << 1,
    LbcConstraintDofTz = 1 << 2,
    LbcConstraintDofRx = 1 << 3,
    LbcConstraintDofRy = 1 << 4,
    LbcConstraintDofRz = 1 << 5,
    LbcConstraintDofAllTrans = LbcConstraintDofTx | LbcConstraintDofTy | LbcConstraintDofTz,
    LbcConstraintDofAllRot = LbcConstraintDofRx | LbcConstraintDofRy | LbcConstraintDofRz,
    LbcConstraintDofAll = 0x3F,
};

class HORNETBASE_EXPORT HILbcConstraint : public HILbc
{
#if !defined(_MSC_VER)
    using Super = HILbc; // required on non-MSVC
#endif
    DECLARE_ITEM_TAGS(HILbcConstraint, CategoryType::CatLbc, ItemType::ItemLbcConstraint)

public:
    HILbcConstraint(Id id, HCursor *cursor, HItemCreatorToken tok);

    DEFINE_TRANSACTION_EXCHANGE(&HILbcConstraint::m_iDof)
public:
    LbcType lbcType() const noexcept override { return LbcType::LbcTypeConstraint; }

    uint32_t dof() const noexcept { return m_iDof; }
    void setDof(LbcConstraintDof dof) noexcept { m_iDof = static_cast<uint32_t>(dof); }
    void addDof(LbcConstraintDof dof) noexcept { m_iDof |= static_cast<uint32_t>(dof); }
    void removeDof(LbcConstraintDof dof) noexcept { m_iDof &= ~static_cast<uint32_t>(dof); }
    bool hasDof(LbcConstraintDof dof) const noexcept { return (m_iDof & static_cast<uint32_t>(dof)) != 0; }

private:
    uint32_t m_iDof;
};