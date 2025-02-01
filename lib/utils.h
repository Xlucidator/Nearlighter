#ifndef UTILS_H
#define UTILS_H

#include <cmath>

// Constant
extern const float infinity;
extern const float pi;

// Utility Function
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

inline float degrees_to_radians(float degrees) {
    return degrees * pi / 180.0f;
}


// Random Generation
#define RANDOM_NEW

#ifdef RANDOM_NEW

#include <random>
extern std::mt19937 gen; // Share the generator: unfit for multi thread now

inline float random_float() {
    static std::uniform_real_distribution<float> dis(0.0f, 1.0f);
    return dis(gen);
}

inline float random_float(float min, float max) {
    static std::uniform_real_distribution<float> dis;
    dis.param(std::uniform_real_distribution<float>::param_type(min, max));
    return dis(gen);
} // TODO: thread local

#else

#include <cstdlib>

inline float random_float() {
    return std::rand() / (RAND_MAX + 1.0);
}

inline float random_float(float min, float max) {
    return min + (max - min) * random_float();
}

#endif


#endif
