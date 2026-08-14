#include <nearlighter/shape/quad.h>

#include <nearlighter/math/constants.h>
#include <nearlighter/sampling/sampler.h>

#include <cmath>
#include <stdexcept>

Quad::Quad(const Point3f& origin, const Vec3f& u, const Vec3f& v)
    : origin_(origin), u_(u), v_(v) {
    const Vec3f area_normal = cross(u_, v_);
    if (area_normal.near_zero()) {
        throw std::invalid_argument("Quad edges must define a non-zero area");
    }
    normal_ = unit_vector(area_normal);
    plane_offset_ = dot(normal_, origin_);
    w_ = area_normal / dot(area_normal, area_normal);
    area_ = area_normal.length();
    bounding_box_ = calculateAABB(origin_, u_, v_);
}

/**
 * @par Implementation
 * The supporting plane satisfies `N dot P = d`. Substituting ray
 * `P(t) = O + tD` gives
 *
 *     t = (d - N dot O) / (N dot D).
 *
 * For plane offset `Q = P - origin` and `W = (u cross v) / |u cross v|^2`,
 * writing `Q = a*u + b*v` gives
 *
 *     Q cross v = a*(u cross v)
 *     u cross Q = b*(u cross v),
 *
 * hence
 *
 *     a = W dot (Q cross v)
 *     b = W dot (u cross Q).
 *
 * The plane hit belongs to the finite Quad exactly when `a,b` lie in `[0,1]`.
 */
bool Quad::hit(const Ray& ray, Interval ray_t, ShapeHit& hit) const {
    /* ----- Supporting-Plane Intersection ----- */
    const float denominator = dot(ray.direction(), normal_);
    if (std::fabs(denominator) < epsilon) return false;

    const float t =
        (plane_offset_ - dot(ray.origin(), normal_)) / denominator;
    if (!ray_t.contains(t)) return false;

    /* ----- Finite Parallelogram Domain ----- */
    const Point3f point = ray.at(t);
    const Vec3f offset = point - origin_;
    const float u_coordinate = dot(w_, cross(offset, v_));
    const float v_coordinate = dot(w_, cross(u_, offset));
    if (!Interval::unit.contains(u_coordinate) ||
        !Interval::unit.contains(v_coordinate)) {
        return false;
    }

    /* ----- Local Surface Interaction ----- */
    hit.t = t;
    hit.point = point;
    hit.geometric_normal = normal_;
    hit.shading_normal = normal_;
    hit.u = u_coordinate;
    hit.v = v_coordinate;
    return true;
}

/**
 * @par Implementation
 * Uniform area density `p_A = 1 / area` converts to solid-angle density
 *
 *     p_omega = distance^2 / (abs(N dot omega) * area).
 */
float Quad::getPDFValue(const Point3f& origin,
                        const Vec3f& direction) const {
    ShapeHit hit_record;
    if (!hit(Ray(origin, direction), Interval(epsilon, infinity),
             hit_record)) {
        return 0.0f;
    }
    const float distance_squared =
        hit_record.t * hit_record.t * direction.length_squared();
    const float cosine = std::fabs(
        dot(unit_vector(direction), hit_record.geometric_normal));
    if (cosine <= 0.0f) return 0.0f;
    return distance_squared / (cosine * area_);
}

Vec3f Quad::random(const Point3f& origin, Sampler& sampler) const {
    const Point3f point = origin_ + u_ * sampler.next1D() +
                          v_ * sampler.next1D();
    return point - origin;
}

AABB Quad::calculateAABB(const Point3f& origin, const Vec3f& u,
                         const Vec3f& v) {
    // Unioning both diagonals encloses all four parallelogram vertices.
    return AABB(AABB(origin, origin + u + v),
                AABB(origin + u, origin + v));
}
