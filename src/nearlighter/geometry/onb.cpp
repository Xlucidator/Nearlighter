#include <nearlighter/geometry/onb.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>

/**
 * @par Implementation
 * 
 * Principle: switch z axis to w, then x, y axes will be the target u, v.
 * 
 * Construction has two steps:
 *
 * 1. Scale before normalization. This avoids overflow or underflow when
 *    computing the squared length.
 * 2. Complete the unit direction `w = (x, y, z)` with Duff's sign-adjusted
 *    closed form:
 *
 *        s = copysign(1, z)
 *        a = -1 / (s + z)
 *        b = x*y*a
 *
 *        u = (1 + s*x*x*a, s*b, -s*x)
 *        v = (b, s + y*y*a, -y)
 *
 * `s` has the sign of `z`, so `|s + z| = 1 + |z|`. The denominator is never
 * near zero, including around the negative z pole.
 *
 * Given `x*x + y*y + z*z = 1`, the formulas satisfy
 *
 *     dot(u, w) = dot(v, w) = dot(u, v) = 0
 *     length(u) = length(v) = 1
 *     cross(u, v) = w
 *
 * No helper-axis branch or second normalization is required.
 *
 * @see Duff et al., "Building an Orthonormal Basis, Revisited", JCGT 2017.
 */
ONB::ONB(const Vec3f& w_direction) {
    if (!std::isfinite(w_direction.x()) ||
        !std::isfinite(w_direction.y()) ||
        !std::isfinite(w_direction.z())) {
        throw std::invalid_argument("ONB direction must be finite");
    }

    const float scale = std::max(
        {std::fabs(w_direction.x()), std::fabs(w_direction.y()),
         std::fabs(w_direction.z())});
    if (scale == 0.0f) {
        throw std::invalid_argument("ONB direction must be non-zero");
    }

    const Vec3f scaled_direction = w_direction / scale;
    w_ = unit_vector(scaled_direction);

    const float sign = std::copysign(1.0f, w_.z());
    const float a = -1.0f / (sign + w_.z());
    const float b = w_.x() * w_.y() * a;

    u_ = Vec3f(1.0f + sign * w_.x() * w_.x() * a,
               sign * b,
               -sign * w_.x());
    v_ = Vec3f(b,
               sign + w_.y() * w_.y() * a,
               -w_.y());
}

Vec3f ONB::toParent(const Vec3f& local) const {
    return local.x() * u_ + local.y() * v_ + local.z() * w_;
}
