#pragma once
#include "HornetBase/HINode.h"

HINode::HINode(Id id, HItemCreatorToken tok)
    : HItem(id, tok, CategoryType::CatNode, ItemType::ItemNode),
      arr_({4, 5, 6, 5, 6})
{
}

const std::array<uint64_t, 5> &HINode::values() const noexcept 
{ 
    return arr_; 
}

std::array<uint64_t, 5> &HINode::values() noexcept 
{ 
    return arr_; 
}

HIElemTest::HIElemTest(Id id, HItemCreatorToken tok)
    : HItem(id, tok, CategoryType::CatElement, ItemType::ItemElementTri6),
      arr_({4, 5, 6, 5, 6, 7})
{
}

const std::array<uint64_t, 6> &HIElemTest::values() const noexcept { return arr_; }
std::array<uint64_t, 6> &HIElemTest::values() noexcept { return arr_; }