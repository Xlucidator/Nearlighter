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

/* Clamp x to the range [min, max]
 *  - clamp_num(x, min, max) : fit for number, range [min, max]
 *  - clamp_idx(x, low, high): fit for index of array, range [low, high)
 */
template<typename T>
inline T clamp_num(T x, T min, T max) {
    if (x < min) return min;
    if (x > max) return max;
    return x;
}

template<typename T>
inline T clamp_idx(T i, T low, T high) {
    if (i < low) return low;
    if (i < high) return i;
    return high - 1;
}



/*=== Interpolation ===*/

/* t in [t0, t1]
 */
inline float smoothstep(float t0, float t1, float t) {
    t = clamp_num((t - t0) / (t1 - t0), 0.0f, 1.0f);
    return t * t * (3 - 2 * t);
}
/* x in [0, 1] 
 */
inline float smoothstep(float x) {
    return x * x * (3 - 2 * x);
}


/* Trilinear Interpolation
 *  simply extend from bilinear interpolation : bilinear interpolation is one layer of trilinear interpolation
 *  => x axis:
 *      linear_interpolate[0][0] = u * c[0][0][0] + (1 - u) * c[0][0][1]
 *      linear_interpolate[0][1] = u * c[0][1][0] + (1 - u) * c[0][1][1]
 *      ---
 *      linear_interpolate[1][0] = u * c[1][0][0] + (1 - u) * c[1][0][1]
 *      linear_interpolate[1][1] = u * c[1][1][0] + (1 - u) * c[1][1][1]
 *  => y axis:
 *      bilinear_interpolate[0] = v * linear_interpolate[0][0] + (1 - v) * linear_interpolate[0][1]
 *                              = c[0][0][0] * u * v + c[0][0][1] * u * (1 - v) + c[0][1][0] * (1 - u) * v + c[0][1][1] * (1 - u) * (1 - v)
 *      bilinear_interpolate[1] = v * linear_interpolate[1][0] + (1 - v) * linear_interpolate[1][1]
 *                              = c[1][0][0] * u * v + c[1][0][1] * u * (1 - v) + c[1][1][0] * (1 - u) * v + c[1][1][1] * (1 - u) * (1 - v)
 *  => z axis:
 *      trilinear_interpolate = w * bilinear_interpolate[0] + (1 - w) * bilinear_interpolate[1]
 *                              = c[0][0][0] * u * v * w + c[0][0][1] * u * v * (1 - w) + c[0][1][0] * u * (1 - v) * w + c[0][1][1] * u * (1 - v) * (1 - w)
 *                              + c[1][0][0] * (1 - u) * v * w + c[1][0][1] * (1 - u) * v * (1 - w) + c[1][1][0] * (1 - u) * (1 - v) * w + c[1][1][1] * (1 - u) * (1 - v) * (1 - w)
 */
inline float trilinear_interpolate(const float c[2][2][2], float u, float v, float w) {
    float res = 0.0f;
    for (int i = 0; i < 2; ++i) 
        for (int j = 0; j < 2; ++j)
            for (int k = 0; k < 2; ++k)
                res += (i * u + (1 - i) * (1 - u)) *
                       (j * v + (1 - j) * (1 - v)) *
                       (k * w + (1 - k) * (1 - w)) * c[i][j][k];
    return res;
}


#endif // MATH_H
