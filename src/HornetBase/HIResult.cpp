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

bool HIResult::getResultComponent(HCursor* target, int component, double& data) const noexcept
{
    auto it = m_mapResult.find(target);
    if (it != m_mapResult.end())
    {
        const auto& resultData = it->second;
        switch (component)
        {
        case 0: data = resultData.displacement.x; return true;
        case 1: data = resultData.displacement.y; return true;
        case 2: data = resultData.displacement.z; return true;
        case 3: data = resultData.displacement.translational; return true;
        case 4: data = resultData.stress.xx; return true;
        case 5: data = resultData.stress.yy; return true;
        case 6: data = resultData.stress.zz; return true;
        case 7: data = resultData.stress.xy; return true;
        case 8: data = resultData.stress.yz; return true;
        case 9: data = resultData.stress.xz; return true;
        case 10: data = resultData.stress.vonMises; return true;
        case 11: data = resultData.strain.xx; return true;
        case 12: data = resultData.strain.yy; return true;
        case 13: data = resultData.strain.zz; return true;
        case 14: data = resultData.strain.xy; return true;
        case 15: data = resultData.strain.yz; return true;
        case 16: data = resultData.strain.xz; return true;
        case 17: data = resultData.strain.vonMises; return true;
        default:
            break;
        }
    }
    return false;
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