#pragma once
#include "HornetBase/HIResult.h"

HIResult::HIResult(Id id, HCursor *cursor, HItemCreatorToken tok)
    : HItem(id, cursor, tok)
{
}

HIResultData HIResult::getResult(HCursor* target) const noexcept
{
    auto it = m_mapResult.find(target);
    if (it != m_mapResult.end())
        return it->second;
    return HIResultData{};
}

void HIResult::setResult(HCursor* target, const HIResultData& data) noexcept
{
    m_mapResult[target] = data;
}

bool HIResult::getDisplacement(HCursor* target, HIResultDataDisplacement& disp) const noexcept
{
    auto it = m_mapResult.find(target);
    if (it != m_mapResult.end())
    {
        disp = it->second.displacement;
        return true;
    }
    return false;
}

bool HIResult::getStress(HCursor* target, HIResultDataStress& stress) const noexcept
{
    auto it = m_mapResult.find(target);
    if (it != m_mapResult.end())
    {
        stress = it->second.stress;
        return true;
    }
    return false;
}

bool HIResult::getStrain(HCursor* target, HIResultDataStrain& strain) const noexcept
{
    auto it = m_mapResult.find(target);
    if (it != m_mapResult.end())
    {
        strain = it->second.strain;
        return true;
    }
    return false;
}

void HIResult::setDisplacement(HCursor* target, const HIResultDataDisplacement& disp) noexcept
{
    m_mapResult[target].displacement = disp;
}

void HIResult::setStress(HCursor* target, const HIResultDataStress& stress) noexcept
{
    m_mapResult[target].stress = stress;
}

void HIResult::setStrain(HCursor* target, const HIResultDataStrain& strain) noexcept
{
    m_mapResult[target].strain = strain;
}