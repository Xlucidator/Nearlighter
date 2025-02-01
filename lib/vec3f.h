#ifndef VEC3F_H
#define VEC3F_H

#include <cmath>
#include <iostream>
#include <random>
#include <assert.h>

#include "utils.h"

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

    bool near_zero() const {
        static float small = 1e-8;
        return (std::fabs(e[0]) < small) && (std::fabs(e[1]) < small) && (std::fabs(e[2]) < small);
    }

    static Vec3f random() {
        return Vec3f(random_float(), random_float(), random_float());
    }

    static Vec3f random(float min, float max) {
        return Vec3f(random_float(min,max), random_float(min,max), random_float(min,max));
    }

    static Vec3f random_unit_vector() {
        static std::normal_distribution<float> dis(0.0f, 1.0f);
        Vec3f v(dis(gen), dis(gen), dis(gen));
        return v /= v.length();
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
    return Vec3f(t * v.e[0], t * v.e[1], t * v.e[2]);
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

inline Vec3f random_unit_on_hemisphere(const Vec3f& normal) {
    Vec3f unit_vec = Vec3f::random_unit_vector();
    return (dot(unit_vec, normal) > 0.0f) ? unit_vec : -unit_vec;
}

inline Vec3f random_unit_in_disk() {
    while (true) {
        Vec3f p(random_float(-1.0f, 1.0f), random_float(-1.0f, 1.0f), 0.0f);
        if (p.length_squared() < 1) return p;
    }
}

inline Vec3f reflect(const Vec3f& vin, const Vec3f& normal) {
    return vin - 2 * dot(vin, normal) * normal;
}

inline Vec3f refract(const Vec3f& vin, const Vec3f& normal, float etai_over_etat) {
    float cos_theta = std::fmin(dot(vin, -normal), 1.0f);
    Vec3f vout_perp = etai_over_etat * (vin + cos_theta * normal);  // component that perpendicular to normal
    Vec3f vout_para = -std::sqrt(std::fabs(1.0f - vout_perp.length_squared())) * normal; // component that parallel to normal
    return vout_perp + vout_para;
}

#endif
