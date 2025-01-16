#ifndef VEC3F_H
#define VEC3F_H

#include <cmath>
#include <iostream>

class Vec3f {
public:
    float e[3];

    Vec3f() : e{0.0f, 0.0f, 0.0f} {}
    Vec3f(float e0, float e1, float e2) : e{e0, e1, e2} {}

    float x() const { return e[0]; }
    float y() const { return e[1]; }
    float z() const { return e[2]; }

    Vec3f operator-() const { return Vec3f(-e[0], -e[1], -e[2]); }
    float operator[](int i) const { return e[i]; }
    float& operator[](int i) { return e[i]; }

    Vec3f& operator+=(const Vec3f& v) {
        e[0] += v.e[0];
        e[1] += v.e[1];
        e[2] += v.e[2];
        return *this;
    }

    Vec3f& operator*=(float t) {
        e[0] *= t;
        e[1] *= t;
        e[2] *= t;
        return *this;
    }

    Vec3f& operator/=(float t) {
        return *this *= 1/t;
    }

    float length() const {
        return std::sqrt(length_squared());
    }

    float length_squared() const {
        return e[0]*e[0] + e[1]*e[1] + e[2]*e[2];
    }
};

typedef Vec3f Point3f;


// Vector Utility Functions
inline std::ostream& operator<<(std::ostream& out, const Vec3f& v) {
    return out << v.e[0] << ' ' << v.e[1] << ' ' << v.e[2];
}

inline Vec3f operator+(const Vec3f& u, const Vec3f& v) {
    return Vec3f(u.e[0] + v.e[0], u.e[1] + v.e[1], u.e[2] + v.e[2]);
}

inline Vec3f operator-(const Vec3f& u, const Vec3f& v) {
    return Vec3f(u.e[0] - v.e[0], u.e[1] - v.e[1], u.e[2] - v.e[2]);
}

inline Vec3f operator*(const Vec3f& u, const Vec3f& v) {
    return Vec3f(u.e[0] * v.e[0], u.e[1] * v.e[1], u.e[2] * v.e[2]);
}

inline Vec3f operator*(float t, const Vec3f& v) {
    return Vec3f(t*v.e[0], t*v.e[1], t*v.e[2]);
}

inline Vec3f operator*(const Vec3f& v, float t) {
    return t * v;
}

inline Vec3f operator/(const Vec3f& v, float t) {
    return (1/t) * v;
}

inline float dot(const Vec3f& u, const Vec3f& v) {
    return u.e[0] * v.e[0]
         + u.e[1] * v.e[1]
         + u.e[2] * v.e[2];
}

inline Vec3f cross(const Vec3f& u, const Vec3f& v) {
    return Vec3f(u.e[1] * v.e[2] - u.e[2] * v.e[1],
                 u.e[2] * v.e[0] - u.e[0] * v.e[2],
                 u.e[0] * v.e[1] - u.e[1] * v.e[0]);
}

inline Vec3f unit_vector(const Vec3f& v) {
    return v / v.length();
}

#endif
