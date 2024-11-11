#ifndef UTILS_H
#define UTILS_H

#include <cmath>

#define PI  (3.1415926535897932385)

/* Solving a x^2 + b x + c = 0
 *   x = (-b +- \sqrt{b^2 - 4ac}) / 2a
 * Optimized: let b = -2h
 *   x = (2h +- \sqrt{4h^2 - 4ac}) / 2a
 *     = (h +- \sqrt{h^2 - ac}) / a
 */
inline bool solveQuadratic(const float& a, const float& b, const float& c,
                           float& x0, float& x1) {
    float h = -0.5 * b;
    float discr_new = h * h - a * c;
    if (discr_new < 0) return false;
    else if (discr_new == 0) x0 = x1 = h / a;
    else {
        float sqrt_discr_new = std::sqrt(discr_new);
        float q = h - std::sqrt(discr_new);
        x0 = q / a;
        x1 = c / q;
    }
    if (x0 > x1) std::swap(x0, x1);
    return true;
}

inline float degrees_to_radians(float degrees) {
    return degrees * PI / 180.0;
}

#endif
