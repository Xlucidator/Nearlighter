#include <nearlighter/shape/box.h>

#include <nearlighter/sampling/sampler.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>

Box::Box(const Point3f& minimum, const Point3f& maximum)
    : minimum_(minimum), maximum_(maximum), extent_(maximum - minimum),
      bounding_box_(minimum, maximum) {
    if (extent_.x() <= 0.0f || extent_.y() <= 0.0f ||
        extent_.z() <= 0.0f) {
        throw std::invalid_argument(
            "Box maximum must exceed minimum on every axis");
    }
    surface_area_ = 2.0f * (extent_.x() * extent_.y() +
                            extent_.x() * extent_.z() +
                            extent_.y() * extent_.z());
}

/**
 * @par Implementation
 * Each axis defines the ray interval between two parallel box planes:
 *
 *     t0 = (minimum[axis] - origin[axis]) / direction[axis]
 *     t1 = (maximum[axis] - origin[axis]) / direction[axis].
 *
 * Intersecting the three intervals gives `[near_t, far_t]`. The plane that
 * raises the lower bound supplies the entry normal; the plane that lowers the
 * upper bound supplies the exit normal. A parallel ray is accepted on an axis
 * only when its origin lies between both planes. Rays starting inside select
 * `far_t`.
 */
bool Box::hit(const Ray& ray, Interval ray_t, ShapeHit& hit) const {
    if (ray.direction().length_squared() == 0.0f) return false;

    float near_t = -infinity;
    float far_t = infinity;
    Vec3f near_normal;
    Vec3f far_normal;

    /* ----- Axis-Slab Intersection ----- */
    for (std::size_t axis = 0; axis < 3; ++axis) {
        const float origin = ray.origin()[axis];
        const float direction = ray.direction()[axis];
        if (direction == 0.0f) {
            if (origin < minimum_[axis] || origin > maximum_[axis]) {
                return false;
            }
            continue;
        }

        float axis_near = (minimum_[axis] - origin) / direction;
        float axis_far = (maximum_[axis] - origin) / direction;
        if (axis_near > axis_far) std::swap(axis_near, axis_far);

        if (axis_near > near_t) {
            near_t = axis_near;
            near_normal = Vec3f();
            near_normal[axis] = direction > 0.0f ? -1.0f : 1.0f;
        }
        if (axis_far < far_t) {
            far_t = axis_far;
            far_normal = Vec3f();
            far_normal[axis] = direction > 0.0f ? 1.0f : -1.0f;
        }
        if (near_t > far_t || near_t > ray_t.max || far_t < ray_t.min) {
            return false;
        }
    }

    /* ----- Accepted Surface ----- */
    const bool use_near = ray_t.contains(near_t);
    const float t = use_near ? near_t : far_t;
    // Inside rays use the exit surface.
    if (!ray_t.contains(t)) return false;

    /* ----- Local Interaction ----- */
    hit.t = t;
    hit.point = ray.at(t);
    hit.geometric_normal = use_near ? near_normal : far_normal;
    hit.shading_normal = hit.geometric_normal;
    calculateUV(hit.point, hit.geometric_normal, hit.u, hit.v);
    return true;
}

/**
 * @par Implementation
 * Uniform area sampling induces
 *
 *     p_omega = distance^2 / (abs(N dot omega) * surface_area).
 *
 * A direction may reach both a near and a far face. Since both surface points
 * map to the same direction, their densities are added as separate preimages.
 */
float Box::getPDFValue(const Point3f& origin,
                       const Vec3f& direction) const {
    /* ----- First Directional Preimage ----- */
    if (direction.length_squared() == 0.0f) return 0.0f;

    const Ray ray(origin, direction);
    ShapeHit near_hit;
    if (!hit(ray, Interval(epsilon, infinity), near_hit)) {
        return 0.0f;
    }

    const Vec3f unit_direction = unit_vector(direction);
    const auto density_for_hit = [&](const ShapeHit& surface_hit) {
        const float cosine = std::fabs(
            dot(unit_direction, surface_hit.geometric_normal));
        if (cosine <= 0.0f) return 0.0f;
        const float distance_squared = surface_hit.t * surface_hit.t *
                                       direction.length_squared();
        return distance_squared / (cosine * surface_area_);
    };

    float density = density_for_hit(near_hit);

    /* ----- Additional Directional Preimage ----- */
    ShapeHit far_hit;
    const float after_near = std::nextafter(near_hit.t, infinity);
    if (hit(ray, Interval(after_near, infinity), far_hit)) {
        density += density_for_hit(far_hit);
    }
    return density;
}

Vec3f Box::random(const Point3f& origin, Sampler& sampler) const {
    /* ----- Area-Proportional Face Selection ----- */
    const float area_xy = extent_.x() * extent_.y();
    const float area_xz = extent_.x() * extent_.z();
    const float area_yz = extent_.y() * extent_.z();
    float selection = sampler.next1D(0.0f, surface_area_);
    const float a = sampler.next1D();
    const float b = sampler.next1D();
    Point3f point;

    /* ----- Uniform Point on Selected Face ----- */
    if ((selection -= area_xy) < 0.0f) {
        point = Point3f(minimum_.x() + a * extent_.x(),
                        minimum_.y() + b * extent_.y(), minimum_.z());
    } else if ((selection -= area_xy) < 0.0f) {
        point = Point3f(minimum_.x() + a * extent_.x(),
                        minimum_.y() + b * extent_.y(), maximum_.z());
    } else if ((selection -= area_xz) < 0.0f) {
        point = Point3f(minimum_.x() + a * extent_.x(), minimum_.y(),
                        minimum_.z() + b * extent_.z());
    } else if ((selection -= area_xz) < 0.0f) {
        point = Point3f(minimum_.x() + a * extent_.x(), maximum_.y(),
                        minimum_.z() + b * extent_.z());
    } else if ((selection -= area_yz) < 0.0f) {
        point = Point3f(minimum_.x(), minimum_.y() + a * extent_.y(),
                        minimum_.z() + b * extent_.z());
    } else {
        point = Point3f(maximum_.x(), minimum_.y() + a * extent_.y(),
                        minimum_.z() + b * extent_.z());
    }
    return point - origin;
}

void Box::calculateUV(const Point3f& point, const Vec3f& normal,
                      float& u, float& v) const {
    if (normal.x() != 0.0f) {
        u = (point.z() - minimum_.z()) / extent_.z();
        v = (point.y() - minimum_.y()) / extent_.y();
    } else if (normal.y() != 0.0f) {
        u = (point.x() - minimum_.x()) / extent_.x();
        v = (point.z() - minimum_.z()) / extent_.z();
    } else {
        u = (point.x() - minimum_.x()) / extent_.x();
        v = (point.y() - minimum_.y()) / extent_.y();
    }
}
