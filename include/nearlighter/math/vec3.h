#ifndef NEARLIGHTER_MATH_VEC3_H
#define NEARLIGHTER_MATH_VEC3_H

#include <cmath>
#include <cstddef>
#include <ostream>
#include <type_traits>

/**
 * Three-component floating-point vector used by geometry and rendering code.
 *
 * Vec3 deliberately supports only float and double. Keeping the scalar type
 * explicit provides the precision choices needed by CPU and future GPU code
 * without introducing a general expression-template math framework.
 */
template <typename T>
class Vec3 {
    static_assert(std::is_same_v<T, float> || std::is_same_v<T, double>,
                  "Vec3 supports only float and double scalars");

public:
    T e[3];

    /** Creates a zero vector. */
    constexpr Vec3() : e{T(0), T(0), T(0)} {}

    /** Creates a vector from Cartesian components. */
    constexpr Vec3(T e0, T e1, T e2) : e{e0, e1, e2} {}

    /** Returns named Cartesian components. */
    constexpr T x() const { return e[0]; }
    constexpr T y() const { return e[1]; }
    constexpr T z() const { return e[2]; }

    constexpr Vec3 operator-() const {
        return Vec3(-e[0], -e[1], -e[2]);
    }
    /** Provides unchecked component access for compact math hot paths. */
    constexpr T operator[](std::size_t i) const { return e[i]; }
    constexpr T& operator[](std::size_t i) { return e[i]; }

    constexpr Vec3& operator+=(const Vec3& v) {
        e[0] += v.e[0];
        e[1] += v.e[1];
        e[2] += v.e[2];
        return *this;
    }

    constexpr Vec3& operator*=(T t) {
        e[0] *= t;
        e[1] *= t;
        e[2] *= t;
        return *this;
    }

    constexpr Vec3& operator/=(T t) {
        return *this *= T(1) / t;
    }

    /** Returns Euclidean vector length. */
    T length() const {
        return std::sqrt(length_squared());
    }

    /** Returns squared length without a square root. */
    constexpr T length_squared() const {
        return e[0] * e[0] + e[1] * e[1] + e[2] * e[2];
    }

    /**
     * Reports whether every component is negligible for geometric fallback.
     *
     * The float threshold preserves the original renderer behavior. Double
     * uses a tighter threshold so selecting it also provides useful precision.
     */
    bool near_zero() const {
        constexpr T small = std::is_same_v<T, float> ? T(1e-8f) : T(1e-12);
        return (std::fabs(e[0]) < small) &&
               (std::fabs(e[1]) < small) &&
               (std::fabs(e[2]) < small);
    }
};

template <typename T>
using Point3 = Vec3<T>;

using Vec3f = Vec3<float>;
using Vec3d = Vec3<double>;
using Point3f = Point3<float>;
using Point3d = Point3<double>;


// Vector Utility Functions
template <typename T>
inline std::ostream& operator<<(std::ostream& out, const Vec3<T>& v) {
    return out << "(" << v.e[0] << ", " << v.e[1] << ", " << v.e[2]
               << ")";
}

template <typename T>
constexpr Vec3<T> operator+(const Vec3<T>& u, const Vec3<T>& v) {
    return Vec3<T>(u.e[0] + v.e[0], u.e[1] + v.e[1], u.e[2] + v.e[2]);
}

template <typename T>
constexpr Vec3<T> operator-(const Vec3<T>& u, const Vec3<T>& v) {
    return Vec3<T>(u.e[0] - v.e[0], u.e[1] - v.e[1], u.e[2] - v.e[2]);
}

template <typename T>
constexpr Vec3<T> operator*(const Vec3<T>& u, const Vec3<T>& v) {
    return Vec3<T>(u.e[0] * v.e[0], u.e[1] * v.e[1], u.e[2] * v.e[2]);
}

template <typename T, typename U,
          std::enable_if_t<std::is_arithmetic_v<U>, int> = 0>
constexpr Vec3<T> operator*(U t, const Vec3<T>& v) {
    const T scalar = static_cast<T>(t);
    return Vec3<T>(scalar * v.e[0], scalar * v.e[1], scalar * v.e[2]);
}

template <typename T, typename U,
          std::enable_if_t<std::is_arithmetic_v<U>, int> = 0>
constexpr Vec3<T> operator*(const Vec3<T>& v, U t) {
    return t * v;
}

template <typename T, typename U,
          std::enable_if_t<std::is_arithmetic_v<U>, int> = 0>
constexpr Vec3<T> operator/(const Vec3<T>& v, U t) {
    return (T(1) / static_cast<T>(t)) * v;
}

template <typename T>
constexpr T dot(const Vec3<T>& u, const Vec3<T>& v) {
    return u.e[0] * v.e[0]
         + u.e[1] * v.e[1]
         + u.e[2] * v.e[2];
}

template <typename T>
constexpr Vec3<T> cross(const Vec3<T>& u, const Vec3<T>& v) {
    return Vec3<T>(u.e[1] * v.e[2] - u.e[2] * v.e[1],
                   u.e[2] * v.e[0] - u.e[0] * v.e[2],
                   u.e[0] * v.e[1] - u.e[1] * v.e[0]);
}

template <typename T>
inline Vec3<T> unit_vector(const Vec3<T>& v) {
    return v / v.length();
}

#endif  // NEARLIGHTER_MATH_VEC3_H
