#pragma once
#include "HornetBase/HILbcForce.h"

HILbcForce::HILbcForce(Id id, HCursor *cursor, HItemCreatorToken tok)
    : HILbc(id, cursor, tok),
    m_vForce(0, 0, 0), m_vMoment(0, 0, 0)
{
}