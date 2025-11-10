#pragma once
#include "HornetBaseExport.h"
#include "HItem.h"
#include "TransactionHelper.h"
#include "HItemManager.h"
#include "HornetUtil/HColor.h"
#include <array>
#include <span>
#include <stdexcept>
#include <algorithm>

enum class ElementType : std::uint16_t
{
    ElementTypeUnkown,
    ElementTypePoint,
    ElementTypeLine2,
    ElementTypeLine3,
    ElementTypeTri3,
    ElementTypeTri6,
    ElementTypeQuad4,
    ElementTypeQuad8,
    ElementTypeTet4,
    ElementTypeTet10,
    ElementTypeHex8,
    ElementTypeHex20,
};

enum class ElementKind : std::uint8_t
{
    ElementKindUnkown,
    ElementKind0D,
    ElementKind1D,
    ElementKind2D,
    ElementKind3D,
};

inline constexpr std::array<ElementKind, 12> kKindOf = {
    ElementKind::ElementKindUnkown, // UNKNOWN
    ElementKind::ElementKind0D,     // POINT
    ElementKind::ElementKind1D,     // LINE2
    ElementKind::ElementKind1D,     // LINE3
    ElementKind::ElementKind2D,     // TRI3
    ElementKind::ElementKind2D,     // TRI6
    ElementKind::ElementKind2D,     // QUAD4
    ElementKind::ElementKind2D,     // QUAD8
    ElementKind::ElementKind3D,     // TET4
    ElementKind::ElementKind3D,     // TET10
    ElementKind::ElementKind3D,     // HEX8
    ElementKind::ElementKind3D,     // HEX20
};

inline constexpr std::array<int, 12> kNodeCount = {
    0,  // UNKNOWN
    1,  // POINT
    2,  // LINE2
    3,  // LINE3
    3,  // TRI3
    6,  // TRI6
    4,  // QUAD4
    8,  // QUAD8
    4,  // TET4
    10, // TET10
    8,  // HEX8
    20, // HEX20
};

constexpr inline bool isElementTypeValid(ElementType t) noexcept
{
    return static_cast<std::size_t>(t) < kNodeCount.size();
}
constexpr inline std::size_t indexOfElementType(ElementType t) noexcept
{
    return static_cast<std::size_t>(t);
}

class HORNETBASE_EXPORT HIElement : public HItem
{
#if !defined(_MSC_VER)
    using Super = HItem; // required on non-MSVC
#endif
    DECLARE_ITEM_VARIANT_ABSTRACT_TAGS(HIElement, CategoryType::CatElement, ItemType::ItemElement, (uint16_t)ElementType::ElementTypeUnkown)

public:
    HIElement(Id id, HCursor *cursor, HItemCreatorToken tok)
        : HItem(id, cursor, tok) {}

    DEFINE_TRANSACTION_EXCHANGE(&HIElement::m_color) 
public:
    virtual ElementType elementType() const noexcept = 0;

    // Connectivity access (view into child's fixed array)
    virtual std::span<HCursor*> nodes() noexcept = 0;
    virtual std::span<HCursor *const> nodes() const noexcept = 0;

    // Convenience
    ElementKind elementKind() const noexcept
    {
        return isElementTypeValid(elementType()) ? kKindOf[indexOfElementType(elementType())] : ElementKind::ElementKindUnkown;
    }

    int nodesCount() noexcept { return static_cast<int>(nodes().size()); }

    // Returns true on success, false if pos is out of range.
    bool setNodeAt(std::size_t pos, HCursor* nodeCursor) noexcept
    {
        auto s = nodes();
        if (pos >= s.size())
            return false;
        s[pos] = nodeCursor;
        return true;
    }

    // Optional helper: bulk assign (copies up to min sizes), returns how many were written.
    std::size_t setNodes(std::span<HCursor*> src) noexcept
    {
        auto dst = nodes();
        const std::size_t n = std::min(src.size(), dst.size());
        std::copy_n(src.begin(), n, dst.begin());
        return n;
    }

private:
    HColor m_color; // color
};

// ===== Concrete elements (templated, zero-initialized storage) =====
template <ElementType T>
class HIElementOf final : public HIElement
{
    static_assert(T != ElementType::ElementTypeUnkown, "Use a known ElementType");
    static constexpr std::size_t N = static_cast<std::size_t>(kNodeCount[indexOfElementType(T)]);
    static_assert(N > 0);

#if !defined(_MSC_VER)
    using Super = HIElement; // required on non-MSVC
#endif
    DECLARE_ITEM_VARIANT_TAGS(HIElementOf, CategoryType::CatElement, ItemType::ItemElement, (uint16_t)T)

public:
    explicit HIElementOf(Id id, HCursor *cursor, HItemCreatorToken tok) : HIElement(id, cursor, tok) {}

    DEFINE_TRANSACTION_EXCHANGE(&HIElementOf::m_arNodes)

    ElementType elementType() const noexcept override { return T; }
    std::span<HCursor *> nodes() noexcept override { return m_arNodes; }
    std::span<HCursor *const> nodes() const noexcept override { return m_arNodes; }

private:
    // Value-initialized => all zeros as requested
    std::array<HCursor *, N> m_arNodes{};
};

// ===== Convenient aliases (optional) =====
using HIElementPoint = HIElementOf<ElementType::ElementTypePoint>;
using HIElementLine2 = HIElementOf<ElementType::ElementTypeLine2>;
using HIElementLine3 = HIElementOf<ElementType::ElementTypeLine3>;
using HIElementTri3 = HIElementOf<ElementType::ElementTypeTri3>;
using HIElementTri6 = HIElementOf<ElementType::ElementTypeTri6>;
using HIElementQuad4 = HIElementOf<ElementType::ElementTypeQuad4>;
using HIElementQuad8 = HIElementOf<ElementType::ElementTypeQuad8>;
using HIElementTet4 = HIElementOf<ElementType::ElementTypeTet4>;
using HIElementTet10 = HIElementOf<ElementType::ElementTypeTet10>;
using HIElementHex8 = HIElementOf<ElementType::ElementTypeHex8>;
using HIElementHex20 = HIElementOf<ElementType::ElementTypeHex20>;

#if 0
class HORNETBASE_EXPORT HIElementTest : public HItem
{
#if !defined(_MSC_VER)
    using Super = HItem; // required on non-MSVC
#endif
    DECLARE_ITEM_TAGS(HIElementTest, CategoryType::CatElement, ItemType::ItemElement)

public:
    HIElementTest(Id id, HCursor *cursor, HItemCreatorToken tok);

    DEFINE_TRANSACTION_EXCHANGE(&HIElementTest::arr_)
public:
    const std::array<uint64_t, 5> &values() const noexcept;
    std::array<uint64_t, 5> &values() noexcept;

private:
    std::array<uint64_t, 5> arr_;
};
#endif