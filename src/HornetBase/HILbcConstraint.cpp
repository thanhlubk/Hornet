#pragma once
#include "HornetBase/HILbcConstraint.h"

HILbcConstraint::HILbcConstraint(Id id, HCursor *cursor, HItemCreatorToken tok)
    : HILbc(id, cursor, tok),
    m_iDof(0)
{
}