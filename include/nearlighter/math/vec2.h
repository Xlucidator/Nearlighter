#ifndef NEARLIGHTER_MATH_VEC2_H
#define NEARLIGHTER_MATH_VEC2_H

#include <cstddef>
#include <type_traits>

/** Two-component floating-point vector used by sampling interfaces. */
template <typename T>
class Vec2 {
    static_assert(std::is_same_v<T, float> || std::is_same_v<T, double>,
                  "Vec2 supports only float and double scalars");

public:
    T e[2];

    constexpr Vec2() : e{T(0), T(0)} {}
    constexpr Vec2(T e0, T e1) : e{e0, e1} {}

    constexpr T x() const { return e[0]; }
    constexpr T y() const { return e[1]; }

    constexpr T operator[](std::size_t index) const { return e[index]; }
    constexpr T& operator[](std::size_t index) { return e[index]; }
};

using Vec2f = Vec2<float>;
using Vec2d = Vec2<double>;

#endif  // NEARLIGHTER_MATH_VEC2_H
