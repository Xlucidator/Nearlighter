#include <nearlighter/shape/sphere.h>

#include <nearlighter/geometry/onb.h>
#include <nearlighter/math/math.h>
#include <nearlighter/sampling/sampler.h>

#include <cmath>
#include <stdexcept>

Sphere::Sphere(const Point3f& center, float radius)
    : center_(center), radius_(radius),
      moving_center_(center, Vec3f(0.0f, 0.0f, 0.0f)) {
    if (radius_ <= 0.0f) {
        throw std::invalid_argument("Sphere radius must be positive");
    }
    bounding_box_ = calculateAABB(center_, radius_);
}

Sphere::Sphere(const Point3f& center_start, const Point3f& center_end,
               float radius)
    : center_(center_start), radius_(radius),
      moving_center_(center_start, center_end - center_start) {
    if (radius_ <= 0.0f) {
        throw std::invalid_argument("Sphere radius must be positive");
    }
    bounding_box_ = AABB(calculateAABB(center_start, radius_),
                         calculateAABB(center_end, radius_));
}

/**
 * @par Implementation
 * For ray `P(t) = O + tD` and sphere `|P - C|^2 = r^2`, let `oc = C - O`:
 *
 *     a = D dot D
 *     h = D dot oc
 *     c = oc dot oc - r^2
 *
 * Substitution gives `a*t^2 - 2*h*t + c = 0`. The roots are tested from
 * nearest to farthest after evaluating the center at the ray time.
 */
bool Sphere::hit(const Ray& ray, Interval ray_t, ShapeHit& hit) const {
    /* ----- Analytic Roots ----- */
    const Point3f current_center = moving_center_.at(ray.time());
    const Vec3f center_offset = current_center - ray.origin();
    const float a = dot(ray.direction(), ray.direction());
    const float h = dot(ray.direction(), center_offset);
    const float c = dot(center_offset, center_offset) - radius_ * radius_;
    float t0;
    float t1;
    if (!solveQuadratic(a, h, c, t0, t1)) return false;
    if (!ray_t.surrounds(t0)) t0 = t1;
    if (!ray_t.surrounds(t0)) return false;

    /* ----- Local Surface Interaction ----- */
    hit.t = t0;
    hit.point = ray.at(t0);
    hit.geometric_normal = (hit.point - current_center) / radius_;
    hit.shading_normal = hit.geometric_normal;
    calculateUV(hit.geometric_normal, hit.u, hit.v);
    return true;
}

/**
 * @par Implementation
 * Every direction from a strictly inside origin reaches the surface. Its
 * support is the complete direction sphere, so
 *
 *     p_omega = 1 / (4*pi).
 *
 * An external origin sees a cone with
 *
 *     cos(theta_max) = sqrt(1 - r^2 / distance^2)
 *     omega = 2*pi*(1 - cos(theta_max)).
 *
 * Uniform cone sampling therefore has constant density `1 / omega`.
 */
float Sphere::getPDFValue(const Point3f& origin,
                          const Vec3f& direction) const {
    if (direction.length_squared() == 0.0f) return 0.0f;

    const float distance_squared =
        (origin - moving_center_.at(0.0f)).length_squared();
    if (distance_squared < radius_ * radius_) {
        return 1.0f / (4.0f * pi);
    }

    ShapeHit hit_record;
    if (!hit(Ray(origin, direction), Interval(epsilon, infinity),
             hit_record)) {
        return 0.0f;
    }

    const float cos_theta_max =
        std::sqrt(1.0f - radius_ * radius_ / distance_squared);
    return 1.0f / (2.0f * pi * (1.0f - cos_theta_max));
}

/**
 * @par Implementation
 * `randomToSphere()` samples a +z-aligned visible cone. An ONB rotates that
 * local direction toward the center. Origins strictly inside the sphere use
 * uniform full-sphere directions because every direction reaches the surface.
 */
Vec3f Sphere::random(const Point3f& origin, Sampler& sampler) const {
    const Vec3f direction = moving_center_.at(0.0f) - origin;
    const float distance_squared = direction.length_squared();
    if (distance_squared < radius_ * radius_) {
        return sampler.nextUnitVector();
    }
    const ONB basis(direction);
    return basis.toParent(
        randomToSphere(radius_, distance_squared, sampler));
}

AABB Sphere::calculateAABB(const Point3f& center, float radius) {
    const Vec3f extent(radius, radius, radius);
    return AABB(center - extent, center + extent);
}

void Sphere::calculateUV(const Point3f& point, float& u, float& v) {
    /*
     * `point` lies on the unit sphere. Polar angle theta and azimuth phi map
     * to `[0,1]^2`; the signs preserve the renderer's established seam and
     * vertical orientation.
     */
    const float theta = std::acos(-point.y());
    const float phi = std::atan2(-point.z(), point.x()) + pi;
    u = phi / (2.0f * pi);
    v = theta / pi;
}

Vec3f Sphere::randomToSphere(float radius, float distance_squared,
                             Sampler& sampler) {
    // Uniform solid angle makes cos(theta) uniform on [cos(theta_max), 1].
    const float r1 = sampler.next1D();
    const float r2 = sampler.next1D();
    const float z = 1.0f + r2 *
        (std::sqrt(1.0f - radius * radius / distance_squared) - 1.0f);
    const float phi = 2.0f * pi * r1;
    const float sin_theta = std::sqrt(1.0f - z * z);
    return Vec3f(sin_theta * std::cos(phi),
                 sin_theta * std::sin(phi), z);
}
