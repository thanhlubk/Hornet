// HColor.hpp
#pragma once
#include <cstdint>
#include <ostream>
#include <climits>

struct HColor
{
    using ColorEncoded = std::uint32_t; // always 32 bits
    ColorEncoded value;                 // stored as 0xAA RR GG BB

    // --- constructors --------------------------------------------------------
    // Default: opaque black (A=255, R=G=B=0)
    constexpr HColor() : value(0xFF00'00'00u) {}

    // From packed 0xAARRGGBB
    explicit constexpr HColor(ColorEncoded argb) : value(argb) {}

    // From RGBA channels (alpha defaults to 255)
    constexpr HColor(std::uint8_t r, std::uint8_t g,
                     std::uint8_t b, std::uint8_t a = 0xFF)
        : value(packRGBA(r, g, b, a)) {}

    // Build from channels (RGBA order with default A=255)
    static constexpr HColor fromRGBA(std::uint8_t r, std::uint8_t g,
                                     std::uint8_t b, std::uint8_t a = 0xFF)
    {
        return HColor(packRGBA(r, g, b, a));
    }
    // Convenience: RGB with default A=255
    static constexpr HColor fromRGB(std::uint8_t r, std::uint8_t g, std::uint8_t b,
                                    std::uint8_t a = 0xFF)
    {
        return fromRGBA(r, g, b, a);
    }
    // Alternative channel order (ARGB)
    static constexpr HColor fromARGB(std::uint8_t a, std::uint8_t r,
                                     std::uint8_t g, std::uint8_t b)
    {
        return HColor(packARGB(a, r, g, b));
    }

    // --- accessors -----------------------------------------------------------
    [[nodiscard]] constexpr std::uint8_t a() const { return std::uint8_t((value >> 24) & 0xFFu); }
    [[nodiscard]] constexpr std::uint8_t r() const { return std::uint8_t((value >> 16) & 0xFFu); }
    [[nodiscard]] constexpr std::uint8_t g() const { return std::uint8_t((value >> 8) & 0xFFu); }
    [[nodiscard]] constexpr std::uint8_t b() const { return std::uint8_t(value & 0xFFu); }

    // Raw packed views
    [[nodiscard]] constexpr ColorEncoded argb() const { return value; } // 0xAARRGGBB
    [[nodiscard]] constexpr ColorEncoded rgba() const
    { // 0xRRGGBBAA
        return (value << 8) | (value >> 24);
    }
    [[nodiscard]] constexpr ColorEncoded rgb() const { return value & 0x00FF'FF'FFu; } // 0x00RRGGBB

    // --- mutators (in-place) -------------------------------------------------
    // Set RGB with optional A (default 255). If you omit A, it uses 255.
    constexpr HColor &setRGB(std::uint8_t r8, std::uint8_t g8, std::uint8_t b8,
                             std::uint8_t a8 = 0xFF)
    {
        value = packRGBA(r8, g8, b8, a8);
        return *this;
    }
    constexpr HColor &setRGBA(std::uint8_t r8, std::uint8_t g8,
                              std::uint8_t b8, std::uint8_t a8 = 0xFF)
    {
        return setRGB(r8, g8, b8, a8);
    }
    constexpr HColor &setARGB(std::uint8_t a8, std::uint8_t r8,
                              std::uint8_t g8, std::uint8_t b8)
    {
        value = packARGB(a8, r8, g8, b8);
        return *this;
    }

    constexpr HColor &setA(std::uint8_t a8)
    {
        value = (value & 0x00FF'FF'FFu) | (ColorEncoded(a8) << 24);
        return *this;
    }
    constexpr HColor &setR(std::uint8_t r8)
    {
        value = (value & 0xFF00'FFFFu) | (ColorEncoded(r8) << 16);
        return *this;
    }
    constexpr HColor &setG(std::uint8_t g8)
    {
        value = (value & 0xFFFF'00FFu) | (ColorEncoded(g8) << 8);
        return *this;
    }
    constexpr HColor &setB(std::uint8_t b8)
    {
        value = (value & 0xFFFF'FF00u) | ColorEncoded(b8);
        return *this;
    }

    // Functional (non-mutating) variants; A defaults to 255 when omitted
    [[nodiscard]] constexpr HColor withRGB(std::uint8_t r8, std::uint8_t g8,
                                           std::uint8_t b8, std::uint8_t a8 = 0xFF) const
    {
        return fromRGBA(r8, g8, b8, a8);
    }
    [[nodiscard]] constexpr HColor withA(std::uint8_t a8) const
    {
        HColor c = *this;
        c.setA(a8);
        return c;
    }
    [[nodiscard]] constexpr HColor withR(std::uint8_t r8) const
    {
        HColor c = *this;
        c.setR(r8);
        return c;
    }
    [[nodiscard]] constexpr HColor withG(std::uint8_t g8) const
    {
        HColor c = *this;
        c.setG(g8);
        return c;
    }
    [[nodiscard]] constexpr HColor withB(std::uint8_t b8) const
    {
        HColor c = *this;
        c.setB(b8);
        return c;
    }

    // --- comparison ----------------------------------------------------------
    constexpr bool operator==(const HColor &) const = default;
    constexpr bool operator!=(const HColor &) const = default;

    // --- packing helpers -----------------------------------------------------
    static constexpr ColorEncoded packRGBA(std::uint8_t r, std::uint8_t g,
                                         std::uint8_t b, std::uint8_t a = 0xFF)
    {
        return (ColorEncoded(a) << 24) | (ColorEncoded(r) << 16) | (ColorEncoded(g) << 8) | ColorEncoded(b);
    }
    static constexpr ColorEncoded packARGB(std::uint8_t a, std::uint8_t r,
                                         std::uint8_t g, std::uint8_t b)
    {
        return (ColorEncoded(a) << 24) | (ColorEncoded(r) << 16) | (ColorEncoded(g) << 8) | ColorEncoded(b);
    }
};

static_assert(CHAR_BIT == 8, "HColor requires 8-bit bytes");
static_assert(sizeof(HColor) == sizeof(HColor::ColorEncoded), "HColor must be 4 bytes");
