#include <nearlighter/scene/surface_interaction.h>

#include <nearlighter/scene/intersectable.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace {

bool isFinite(const Vec3f& value) {
    return std::isfinite(value.x()) && std::isfinite(value.y()) &&
           std::isfinite(value.z());
}

Vec3f validatedOutgoing(const Ray& ray) {
    if (!isFinite(ray.direction()) || ray.direction().near_zero()) {
        throw std::invalid_argument(
            "SurfaceInteraction requires a finite non-zero incident ray");
    }
    return -unit_vector(ray.direction());
}

}  // namespace

SurfaceInteraction::SurfaceInteraction(const Ray& incident_ray,
                                       const HitRecord& record)
    : point_(record.point),
      geometric_normal_(record.geometric_normal),
      shading_normal_(record.normal),
      outgoing_(validatedOutgoing(incident_ray)),
      ray_parameter_(record.t),
      u_(record.u),
      v_(record.v),
      front_face_(record.front_face),
      primitive_(record.primitive),
      material_(record.material),
      frame_(record.normal) {
    if (record.kind != InteractionKind::Surface || !primitive_ || !material_) {
        throw std::invalid_argument(
            "SurfaceInteraction requires a complete surface record");
    }
}

/**
 * @par Implementation
 * Offsets along the face-forward geometric normal toward the outgoing side.
 * The scale follows the point magnitude so the displacement remains larger
 * than local float rounding across scene scales. `nextafter` then advances
 * every participating component beyond the rounded offset position.
 */
Ray SurfaceInteraction::spawnRay(const Vec3f& direction, float time) const {
    if (!isFinite(direction) || direction.near_zero()) {
        throw std::invalid_argument(
            "Surface ray direction must be finite and non-zero");
    }

    const float point_scale = std::max(
        {1.0f, std::fabs(point_.x()), std::fabs(point_.y()),
         std::fabs(point_.z())});
    const float magnitude = 32.0f * std::numeric_limits<float>::epsilon() *
                            point_scale;
    const float side = dot(direction, geometric_normal_) >= 0.0f ? 1.0f
                                                                 : -1.0f;
    const Vec3f offset = side * magnitude * geometric_normal_;
    Point3f origin = point_ + offset;

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
