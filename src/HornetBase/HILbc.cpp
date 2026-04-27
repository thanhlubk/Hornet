#pragma once
#include "HornetBase/HILbc.h"

HILbc::HILbc(Id id, HCursor *cursor, HItemCreatorToken tok)
    : HItem(id, cursor, tok)
{
    m_vecTarget.clear();
}