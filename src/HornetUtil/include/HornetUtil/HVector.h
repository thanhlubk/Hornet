#pragma once
#include <cassert>
#include <cmath>
#include <cstddef>
#include <type_traits>
#include <limits>
#include <ostream>
#include "HornetUtilExport.h"

template <class T> 
struct HVector3
{
    static_assert(std::is_arithmetic_v<T>, "HVector3<T>: T must be arithmetic");

    union
    {
        struct { T x, y, z; };
        T data[3];
    };

    // ---- ctors --------------------------------------------------------------
    constexpr HVector3() : x(T{}), y(T{}), z(T{}) {}
    constexpr HVector3(T x_, T y_, T z_) : x(x_), y(y_), z(z_) {}

    template <class U>
    explicit constexpr HVector3(const HVector3<U> &v)
        : x(static_cast<T>(v.x)), y(static_cast<T>(v.y)), z(static_cast<T>(v.z)) {}

    // ---- element access -----------------------------------------------------
    constexpr T &operator[](std::size_t i)
    {
        assert(i < 3);
        return data[i];
    }
    constexpr const T &operator[](std::size_t i) const
    {
        assert(i < 3);
        return data[i];
    }

    // ---- comparison ---------------------------------------------------------
    constexpr bool operator==(const HVector3 &rhs) const
    {
        return x == rhs.x && y == rhs.y && z == rhs.z;
    }
    constexpr bool operator!=(const HVector3 &rhs) const { return !(*this == rhs); }

    // Optional: approximate equality for floating-point
    constexpr bool approxEqual(const HVector3 &rhs,
                               T eps = static_cast<T>(1e-6)) const
        requires std::is_floating_point_v<T>
    {
        return (std::fabs(x - rhs.x) <= eps) &&
               (std::fabs(y - rhs.y) <= eps) &&
               (std::fabs(z - rhs.z) <= eps);
    }

    // ---- unary --------------------------------------------------------------
    constexpr HVector3 operator+() const { return *this; }
    constexpr HVector3 operator-() const { return HVector3(-x, -y, -z); }

    // ---- vector + vector ----------------------------------------------------
    constexpr HVector3 &operator+=(const HVector3 &r)
    {
        x += r.x;
        y += r.y;
        z += r.z;
        return *this;
    }
    constexpr HVector3 &operator-=(const HVector3 &r)
    {
        x -= r.x;
        y -= r.y;
        z -= r.z;
        return *this;
    }
    friend constexpr HVector3 operator+(HVector3 l, const HVector3 &r)
    {
        l += r;
        return l;
    }
    friend constexpr HVector3 operator-(HVector3 l, const HVector3 &r)
    {
        l -= r;
        return l;
    }

    // ---- scalar ops (vector * scalar, vector / scalar, scalar * vector) ----
    template <class S, std::enable_if_t<std::is_arithmetic_v<S>, int> = 0>
    constexpr HVector3<std::common_type_t<T, S>> operator*(S s) const
    {
        using R = std::common_type_t<T, S>;
        return {static_cast<R>(x) * s, static_cast<R>(y) * s, static_cast<R>(z) * s};
    }

    template <class S, std::enable_if_t<std::is_arithmetic_v<S>, int> = 0>
    constexpr HVector3 &operator*=(S s)
    {
        x = T(x * s);
        y = T(y * s);
        z = T(z * s);
        return *this;
    }

    template <class S, std::enable_if_t<std::is_arithmetic_v<S>, int> = 0>
    constexpr HVector3<std::common_type_t<T, S>> operator/(S s) const
    {
        using R = std::common_type_t<T, S>;
        return {static_cast<R>(x) / s, static_cast<R>(y) / s, static_cast<R>(z) / s};
    }

    template <class S, std::enable_if_t<std::is_arithmetic_v<S>, int> = 0>
    constexpr HVector3 &operator/=(S s)
    {
        x = T(x / s);
        y = T(y / s);
        z = T(z / s);
        return *this;
    }

    template <class S, std::enable_if_t<std::is_arithmetic_v<S>, int> = 0>
    friend constexpr HVector3<std::common_type_t<T, S>> operator*(S s, const HVector3 &v)
    {
        return v * s;
    }

    // ---- dot / cross --------------------------------------------------------
    template <class U>
    constexpr auto dot(const HVector3<U> &r) const -> std::common_type_t<T, U>
    {
        using R = std::common_type_t<T, U>;
        return R(x) * r.x + R(y) * r.y + R(z) * r.z;
    }

    template <class U>
    constexpr auto cross(const HVector3<U> &r) const -> HVector3<std::common_type_t<T, U>>
    {
        using R = std::common_type_t<T, U>;
        return {
            R(y) * r.z - R(z) * r.y,
            R(z) * r.x - R(x) * r.z,
            R(x) * r.y - R(y) * r.x};
    }

    // ---- length / normalize -------------------------------------------------
    // lengthSquared returns in the arithmetic type (or promoted)
    constexpr auto lengthSquared() const -> std::conditional_t<std::is_floating_point_v<T>, T, long double>
    {
        using R = std::conditional_t<std::is_floating_point_v<T>, T, long double>;
        return R(x) * R(x) + R(y) * R(y) + R(z) * R(z);
    }

    auto length() const -> std::conditional_t<std::is_floating_point_v<T>, T, long double>
    {
        using R = std::conditional_t<std::is_floating_point_v<T>, T, long double>;
        return static_cast<R>(std::sqrt(static_cast<long double>(lengthSquared())));
    }

    // normalize in-place; no-op for zero-length
    HVector3 &normalize()
    {
        auto len = length();
        if (len != decltype(len)(0))
        {
            x = static_cast<T>(x / len);
            y = static_cast<T>(y / len);
            z = static_cast<T>(z / len);
        }
        return *this;
    }

    // return a normalized copy (returns zero vector for zero-length input)
    [[nodiscard]] HVector3 normalized() const
    {
        auto len = length();
        if (len == decltype(len)(0))
            return *this;
        return {static_cast<T>(x / len), static_cast<T>(y / len), static_cast<T>(z / len)};
    }

    // ---- helpers ------------------------------------------------------------
    static constexpr std::size_t size() { return 3; }
    constexpr T *begin() { return &data[0]; }
    constexpr T *end() { return &data[0] + 3; }
    constexpr const T *begin() const { return &data[0]; }
    constexpr const T *end() const { return &data[0] + 3; }
};

// extern-template declarations (match your HVector3 pattern)
#if !defined(HORNETUTIL_LIBRARY)
extern template struct HORNETUTIL_EXPORT HVector3<float>;
extern template struct HORNETUTIL_EXPORT HVector3<double>;
extern template struct HORNETUTIL_EXPORT HVector3<int>;
#endif

// convenient aliases
using HVector3f = HVector3<float>;
using HVector3d = HVector3<double>;
using HVector3i = HVector3<int>;

// In HVector.h (after HVector3 and its externs)
template <class T>
struct HVector2
{
    static_assert(std::is_arithmetic_v<T>, "HVector2<T>: T must be arithmetic");

    union
    {
        struct
        {
            T x, y;
        };
        T data[2];
    };

    // ---- ctors --------------------------------------------------------------
    constexpr HVector2() : x(T{}), y(T{}) {}
    constexpr HVector2(T x_, T y_) : x(x_), y(y_) {}

    template <class U>
    explicit constexpr HVector2(const HVector2<U> &v)
        : x(static_cast<T>(v.x)), y(static_cast<T>(v.y)) {}

    // ---- element access -----------------------------------------------------
    constexpr T &operator[](std::size_t i)
    {
        assert(i < 2);
        return data[i];
    }
    constexpr const T &operator[](std::size_t i) const
    {
        assert(i < 2);
        return data[i];
    }

    // ---- comparison ---------------------------------------------------------
    constexpr bool operator==(const HVector2 &rhs) const { return x == rhs.x && y == rhs.y; }
    constexpr bool operator!=(const HVector2 &rhs) const { return !(*this == rhs); }

    // Optional: approximate equality for floating-point
    constexpr bool approxEqual(const HVector2 &rhs,
                               T eps = static_cast<T>(1e-6)) const
        requires std::is_floating_point_v<T>
    {
        return (std::fabs(x - rhs.x) <= eps) &&
               (std::fabs(y - rhs.y) <= eps);
    }

    // ---- unary --------------------------------------------------------------
    constexpr HVector2 operator+() const { return *this; }
    constexpr HVector2 operator-() const { return HVector2(-x, -y); }

    // ---- vector + vector ----------------------------------------------------
    constexpr HVector2 &operator+=(const HVector2 &r)
    {
        x += r.x;
        y += r.y;
        return *this;
    }
    constexpr HVector2 &operator-=(const HVector2 &r)
    {
        x -= r.x;
        y -= r.y;
        return *this;
    }
    friend constexpr HVector2 operator+(HVector2 l, const HVector2 &r)
    {
        l += r;
        return l;
    }
    friend constexpr HVector2 operator-(HVector2 l, const HVector2 &r)
    {
        l -= r;
        return l;
    }

    // ---- scalar ops (vector * scalar, vector / scalar, scalar * vector) ----
    template <class S, std::enable_if_t<std::is_arithmetic_v<S>, int> = 0>
    constexpr HVector2<std::common_type_t<T, S>> operator*(S s) const
    {
        using R = std::common_type_t<T, S>;
        return {static_cast<R>(x) * s, static_cast<R>(y) * s};
    }
    template <class S, std::enable_if_t<std::is_arithmetic_v<S>, int> = 0>
    constexpr HVector2 &operator*=(S s)
    {
        x = T(x * s);
        y = T(y * s);
        return *this;
    }

    template <class S, std::enable_if_t<std::is_arithmetic_v<S>, int> = 0>
    constexpr HVector2<std::common_type_t<T, S>> operator/(S s) const
    {
        using R = std::common_type_t<T, S>;
        return {static_cast<R>(x) / s, static_cast<R>(y) / s};
    }
    template <class S, std::enable_if_t<std::is_arithmetic_v<S>, int> = 0>
    constexpr HVector2 &operator/=(S s)
    {
        x = T(x / s);
        y = T(y / s);
        return *this;
    }

    template <class S, std::enable_if_t<std::is_arithmetic_v<S>, int> = 0>
    friend constexpr HVector2<std::common_type_t<T, S>> operator*(S s, const HVector2 &v)
    {
        return v * s;
    }

    // ---- dot / "cross" ------------------------------------------------------
    template <class U>
    constexpr auto dot(const HVector2<U> &r) const -> std::common_type_t<T, U>
    {
        using R = std::common_type_t<T, U>;
        return R(x) * r.x + R(y) * r.y;
    }

    // 2D "cross" = z-component of 3D cross (a.x*b.y - a.y*b.x), returns a scalar
    template <class U>
    constexpr auto cross(const HVector2<U> &r) const -> std::common_type_t<T, U>
    {
        using R = std::common_type_t<T, U>;
        return R(x) * r.y - R(y) * r.x;
    }

    // ---- length / normalize -------------------------------------------------
    constexpr auto lengthSquared() const
        -> std::conditional_t<std::is_floating_point_v<T>, T, long double>
    {
        using R = std::conditional_t<std::is_floating_point_v<T>, T, long double>;
        return R(x) * R(x) + R(y) * R(y);
    }

    auto length() const
        -> std::conditional_t<std::is_floating_point_v<T>, T, long double>
    {
        using R = std::conditional_t<std::is_floating_point_v<T>, T, long double>;
        return static_cast<R>(std::sqrt(static_cast<long double>(lengthSquared())));
    }

    HVector2 &normalize()
    {
        auto len = length();
        if (len != decltype(len)(0))
        {
            x = static_cast<T>(x / len);
            y = static_cast<T>(y / len);
        }
        return *this;
    }

    [[nodiscard]] HVector2 normalized() const
    {
        auto len = length();
        if (len == decltype(len)(0))
            return *this;
        return {static_cast<T>(x / len), static_cast<T>(y / len)};
    }

    // ---- helpers ------------------------------------------------------------
    static constexpr std::size_t size() { return 2; }
    constexpr T *begin() { return &data[0]; }
    constexpr T *end() { return &data[0] + 2; }
    constexpr const T *begin() const { return &data[0]; }
    constexpr const T *end() const { return &data[0] + 2; }
};

#if !defined(HORNETUTIL_LIBRARY)
extern template struct HORNETUTIL_EXPORT HVector2<float>;
extern template struct HORNETUTIL_EXPORT HVector2<double>;
extern template struct HORNETUTIL_EXPORT HVector2<int>;
#endif

// convenient aliases
using HVector2f = HVector2<float>;
using HVector2d = HVector2<double>;
using HVector2i = HVector2<int>;
