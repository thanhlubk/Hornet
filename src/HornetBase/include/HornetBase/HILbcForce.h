#pragma once
#include "HornetBaseExport.h"
#include "HILbc.h"
#include "HornetUtil/HVector.h"

class HORNETBASE_EXPORT HILbcForce : public HILbc
{
#if !defined(_MSC_VER)
    using Super = HILbc; // required on non-MSVC
#endif
    DECLARE_ITEM_TAGS(HILbcForce, CategoryType::CatLbc, ItemType::ItemLbcForce)

public:
    HILbcForce(Id id, HCursor *cursor, HItemCreatorToken tok);

    DEFINE_TRANSACTION_EXCHANGE(&HILbcForce::m_vForce,
                                &HILbcForce::m_vMoment)
public:
    LbcType lbcType() const noexcept override { return LbcType::LbcTypeForce; }

    HVector3d force() const noexcept { return m_vForce; }
    HVector3d moment() const noexcept { return m_vMoment; }

    void setForce(const HVector3d &force) noexcept { m_vForce = force; }
    void setMoment(const HVector3d &moment) noexcept { m_vMoment = moment; }

private:
    HVector3d m_vForce; // force
    HVector3d m_vMoment; // moment
};