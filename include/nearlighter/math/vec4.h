#ifndef NEARLIGHTER_MATH_VEC4_H
#define NEARLIGHTER_MATH_VEC4_H

#include <nearlighter/math/vec3.h>

#include <cmath>
#include <cstddef>
#include <ostream>
#include <type_traits>

/**
 * Four-component floating-point vector for homogeneous and graphics data.
 *
 * Vec4 is intentionally independent from Mat4: it can represent arbitrary
 * four-component values, while point/vector transformation policy remains in
 * the matrix API. As with Vec3, only float and double scalar types are valid.
 */
template <typename T>
class Vec4 {
    static_assert(std::is_same_v<T, float> || std::is_same_v<T, double>,
                  "Vec4 supports only float and double scalars");

public:
    T e[4];

    /** Creates a zero vector. */
    constexpr Vec4() : e{T(0), T(0), T(0), T(0)} {}

    /** Creates a vector from four Cartesian or homogeneous components. */
    constexpr Vec4(T e0, T e1, T e2, T e3) : e{e0, e1, e2, e3} {}

    /** Appends one component to a Vec3 of the same scalar type. */
    constexpr Vec4(const Vec3<T>& xyz, T w)
        : e{xyz.x(), xyz.y(), xyz.z(), w} {}

    /** Returns named components. */
    constexpr T x() const { return e[0]; }
    constexpr T y() const { return e[1]; }
    constexpr T z() const { return e[2]; }
    constexpr T w() const { return e[3]; }
    /** Returns the first three components as an independent Vec3. */
    constexpr Vec3<T> xyz() const { return Vec3<T>(e[0], e[1], e[2]); }

    constexpr Vec4 operator-() const {
        return Vec4(-e[0], -e[1], -e[2], -e[3]);
    }
    /** Provides unchecked component access for compact math hot paths. */
    constexpr T operator[](std::size_t i) const { return e[i]; }
    constexpr T& operator[](std::size_t i) { return e[i]; }

    constexpr Vec4& operator+=(const Vec4& v) {
        e[0] += v.e[0];
        e[1] += v.e[1];
        e[2] += v.e[2];
        e[3] += v.e[3];
        return *this;
    }

    constexpr Vec4& operator*=(T t) {
        e[0] *= t;
        e[1] *= t;
        e[2] *= t;
        e[3] *= t;
        return *this;
    }

    constexpr Vec4& operator/=(T t) {
        return *this *= T(1) / t;
    }

    /** Returns Euclidean four-dimensional vector length. */
    T length() const { return std::sqrt(length_squared()); }

    /** Returns squared length without a square root. */
    constexpr T length_squared() const {
        return e[0] * e[0] + e[1] * e[1] +
               e[2] * e[2] + e[3] * e[3];
    }
};

using Vec4f = Vec4<float>;
using Vec4d = Vec4<double>;

// Vector Utility Functions
template <typename T>
inline std::ostream& operator<<(std::ostream& out, const Vec4<T>& v) {
    return out << "(" << v.e[0] << ", " << v.e[1] << ", " << v.e[2]
               << ", " << v.e[3] << ")";
}

template <typename T>
constexpr Vec4<T> operator+(const Vec4<T>& u, const Vec4<T>& v) {
    return Vec4<T>(u.e[0] + v.e[0], u.e[1] + v.e[1],
                   u.e[2] + v.e[2], u.e[3] + v.e[3]);
}

template <typename T>
constexpr Vec4<T> operator-(const Vec4<T>& u, const Vec4<T>& v) {
    return Vec4<T>(u.e[0] - v.e[0], u.e[1] - v.e[1],
                   u.e[2] - v.e[2], u.e[3] - v.e[3]);
}

template <typename T>
constexpr Vec4<T> operator*(const Vec4<T>& u, const Vec4<T>& v) {
    return Vec4<T>(u.e[0] * v.e[0], u.e[1] * v.e[1],
                   u.e[2] * v.e[2], u.e[3] * v.e[3]);
}

template <typename T, typename U,
          std::enable_if_t<std::is_arithmetic_v<U>, int> = 0>
constexpr Vec4<T> operator*(U t, const Vec4<T>& v) {
    const T scalar = static_cast<T>(t);
    return Vec4<T>(scalar * v.e[0], scalar * v.e[1],
                   scalar * v.e[2], scalar * v.e[3]);
}

template <typename T, typename U,
          std::enable_if_t<std::is_arithmetic_v<U>, int> = 0>
constexpr Vec4<T> operator*(const Vec4<T>& v, U t) {
    return t * v;
}

template <typename T, typename U,
          std::enable_if_t<std::is_arithmetic_v<U>, int> = 0>
constexpr Vec4<T> operator/(const Vec4<T>& v, U t) {
    return (T(1) / static_cast<T>(t)) * v;
}

template <typename T>
constexpr T dot(const Vec4<T>& u, const Vec4<T>& v) {
    return u.e[0] * v.e[0] + u.e[1] * v.e[1] +
           u.e[2] * v.e[2] + u.e[3] * v.e[3];
}

template <typename T>
inline Vec4<T> unit_vector(const Vec4<T>& v) {
    return v / v.length();
}

#endif  // NEARLIGHTER_MATH_VEC4_H
