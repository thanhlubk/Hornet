#pragma once
#include "HornetBaseExport.h"
#include "HItem.h"
#include "TransactionHelper.h"
#include "HItemManager.h"

struct HIResultDataDisplacement
{
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

struct HIResultDataStress
{
    double xx = 0.0;
    double yy = 0.0;
    double zz = 0.0;
    double xy = 0.0;
    double yz = 0.0;
    double xz = 0.0;
    double vonMises = 0.0;
};

struct HIResultDataStrain
{
    double xx = 0.0;
    double yy = 0.0;
    double zz = 0.0;
    double xy = 0.0;
    double yz = 0.0;
    double xz = 0.0;
};

struct HIResultData
{
    HIResultDataDisplacement displacement;
    HIResultDataStress stress;
    HIResultDataStrain strain;
};

class HORNETBASE_EXPORT HIResult : public HItem
{
#if !defined(_MSC_VER)
    using Super = HItem; // required on non-MSVC
#endif
    DECLARE_ITEM_TAGS(HIResult, CategoryType::CatResult, ItemType::ItemResult)

public:
    HIResult(Id id, HCursor *cursor, HItemCreatorToken tok);

    DEFINE_TRANSACTION_EXCHANGE(&HIResult::m_mapResult)
    
public:
    HIResultData getResult(HCursor* target) const noexcept;
    void setResult(HCursor* target, const HIResultData& data) noexcept;

    bool getDisplacement(HCursor* target, HIResultDataDisplacement& disp) const noexcept;
    bool getStress(HCursor* target, HIResultDataStress& stress) const noexcept;
    bool getStrain(HCursor* target, HIResultDataStrain& strain) const noexcept;

    void setDisplacement(HCursor* target, const HIResultDataDisplacement& disp) noexcept;
    void setStress(HCursor* target, const HIResultDataStress& stress) noexcept;
    void setStrain(HCursor* target, const HIResultDataStrain& strain) noexcept;

    int step() const noexcept { return m_iStep; }
    void setStep(int step) noexcept { m_iStep = step; }

    int count() const noexcept { return static_cast<int>(m_mapResult.size()); }

private:
    int m_iStep = 0;
    std::unordered_map<HCursor*, HIResultData> m_mapResult; // target connectivity (variable size, but fixed per derived type)
};
