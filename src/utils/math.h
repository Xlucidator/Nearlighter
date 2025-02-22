#ifndef MATH_H
#define MATH_H

#include <cmath>

#include "constant.h"

// Utility Function
inline float degrees_to_radians(float degrees) {
    return degrees * pi / 180.0f;
}

/* Solving a x^2 + b x + c = 0
 *   x = (-b +- \sqrt{b^2 - 4ac}) / 2a
 * Optimized: let b = -2h
 *   x = (2h +- \sqrt{4h^2 - 4ac}) / 2a
 *     = (h +- \sqrt{h^2 - ac}) / a
 */
inline bool solveQuadratic(const float& a, const float& h, const float& c,
                           float& x0, float& x1) {
    float discr_new = h * h - a * c;
    if (discr_new < 0) return false;
    else if (discr_new == 0) x0 = x1 = h / a;
    else {
        float sqrt_d = std::sqrt(discr_new);
        x0 = (h - sqrt_d) / a;
        x1 = (h + sqrt_d) / a;
    }
    if (x0 > x1) std::swap(x0, x1);
    return true;
}

// Template
template<typename T>
inline T clamp(T x, T min, T max) {
    if (x < min) return min;
    if (x > max) return max;
    return x;
}

#endif // MATH_H