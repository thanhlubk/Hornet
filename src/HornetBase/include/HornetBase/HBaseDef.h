#pragma once
#include <cstddef> // std::byte
#include <memory>
#include <new>
#include <cstring> // memcpy
#include <array>
#include <optional>
#include <type_traits>

enum class CategoryType : std::uint16_t
{
    CatUnknown = 0,
    CatNode,
    CatElement,
    CatGroup,
    CatLbc,

    // Add above this
    CatEnd,
};

enum class ItemType : std::uint16_t
{
    ItemUnkown,
    ItemNode,
    ItemNodeHeat,
    ItemElement,
    ItemLbc,
    ItemLbcForce,
    ItemLbcConstraint,

    // Add above this
    ItemEnd,
};

using Id = std::uint64_t;

inline std::size_t alignUp(std::size_t offset, std::size_t alignment)
{
    const std::size_t mask = alignment - 1;
    return (offset + mask) & ~mask;
}
