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

    // Add above this
    CatEnd,
};

enum class ItemType : std::uint16_t
{
    ItemUnkown,
    ItemNode,
    ItemElement,
    ItemElementPoint,
    ItemElementLine2,
    ItemElementLine3,
    ItemElementTri3,
    ItemElementTri6,
    ItemElementQuad4,
    ItemElementQuad8,
    ItemElementTet4,
    ItemElementTet10,
    ItemElementHex8,
    ItemElementHex20,
    ItemElementPrism6,
    ItemElementPrism15,

    // Add above this
    ItemEnd,
};

using Id = std::uint64_t;

inline std::size_t alignUp(std::size_t offset, std::size_t alignment)
{
    const std::size_t mask = alignment - 1;
    return (offset + mask) & ~mask;
}
