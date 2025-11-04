#pragma once
#include "HornetBase/HINode.h"

HINode::HINode(Id id, HCursor *cursor, HItemCreatorToken tok)
    : HItem(id, cursor, tok),
    m_vPosition(0, 0, 0), m_color(0, 0, 0, 0)
{
}

HINodeHeat::HINodeHeat(Id id, HCursor *cursor, HItemCreatorToken tok)
    : HINode(id, cursor, tok),
    m_temperature(0.0)
{
}