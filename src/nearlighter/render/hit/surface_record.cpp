#include <nearlighter/render/hit/surface_record.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace {
bool isFinite(const Vec3f& value) {
    return std::isfinite(value.x()) && std::isfinite(value.y()) &&
           std::isfinite(value.z());
}
}  // namespace

/**
 * @par Implementation
 * Offsets along the face-forward geometric normal toward the outgoing side.
 * Point magnitude sets an offset larger than local float rounding.
 * nextafter advances participating components beyond the rounded position.
 */
Ray SurfaceRecord::spawnRay(const Vec3f& direction, float time) const {
    if (!isFinite(direction) || direction.near_zero()) {
        throw std::invalid_argument(
            "Surface ray direction must be finite and non-zero");
    }

    const float point_scale = std::max(
        {1.0f, std::fabs(point.x()), std::fabs(point.y()),
         std::fabs(point.z())});
    const float magnitude = 32.0f * std::numeric_limits<float>::epsilon() *
                            point_scale;
    const float side = dot(direction, geometric_normal) >= 0.0f ? 1.0f
                                                                 : -1.0f;
    const Vec3f offset = side * magnitude * geometric_normal;
    Point3f origin = point + offset;

    for (std::size_t axis = 0; axis < 3; ++axis) {
        if (offset[axis] > 0.0f) {
            origin[axis] = std::nextafter(
                origin[axis], std::numeric_limits<float>::infinity());
        } else if (offset[axis] < 0.0f) {
            origin[axis] = std::nextafter(
                origin[axis], -std::numeric_limits<float>::infinity());
        }
    }
    return Ray(origin, direction, time);
}
