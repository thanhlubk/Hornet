#pragma once
#include "HornetBase/HIElement.h"

HIElement::HIElement(Id id, HCursor *cursor, HItemCreatorToken tok)
    : HItem(id, cursor, tok),
    m_color(0, 0, 255, 255), m_eTypeExtended(ElementTypeExtended::ElementTypeExtendedStandard)
{
}