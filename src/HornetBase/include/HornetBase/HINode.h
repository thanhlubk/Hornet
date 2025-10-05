#pragma once
#include "HornetBaseExport.h"
#include "HItem.h"
#include "TransactionHelper.h"
#include "HItemManager.h"
#include <array>

class HORNETBASE_EXPORT HINode : public HItem
{
    DECLARE_ITEM_STATIC_TAGS(HINode, CategoryType::CatNode, ItemType::ItemNode)

public:
    HINode(Id id, HItemCreatorToken tok);

    DEFINE_TRANSACTION_EXCHANGE(HINode,
                          &HINode::arr_)
public:
    const std::array<uint64_t, 5> &values() const noexcept;
    std::array<uint64_t, 5> &values() noexcept;

private:
    std::array<uint64_t, 5> arr_;
};

class HORNETBASE_EXPORT HIElemTest : public HItem
{
    DECLARE_ITEM_STATIC_TAGS(HIElemTest, CategoryType::CatElement, ItemType::ItemElementTri6)

public:
    HIElemTest(Id id, HItemCreatorToken tok);
    DEFINE_TRANSACTION_EXCHANGE(HIElemTest,
                                &HIElemTest::arr_)
public:
    const std::array<uint64_t, 6> &values() const noexcept;
    std::array<uint64_t, 6> &values() noexcept;

private:
    std::array<uint64_t, 6> arr_;
};
