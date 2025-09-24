#include "sphere.h"
#include "utils/math.h"

/** 
 * Hit a Sphere Primitive
 * 
 * Ray: o + t \vec{d}, Sphere: (p - c)^2 = r^2
 * Hit:
 *  => (o + t \vec{d} - c)^2 = r^2
 *  => \vec{d}^2 t^2 + 2 \vec{d} (o-c) t + (o-c)^2 - r^2 = 0
 *  => Solving the Quadratic equation: a = \vec{d}^2, b = 2\vec{d}(o-c), c = (o-c)^2-r^2
 *  => Solution t should in the ray bound
 */
bool Sphere::hit(const Ray& r, Interval ray_t, HitRecord& hit_record) const {
    Point3f current_center = moving_center.at(r.time());
    Vec3f oc = current_center - r.origin();
    
    float a = dot(r.direction(), r.direction());
    float h = dot(r.direction(), oc);  // h = - b / 2
    float c = dot(oc, oc) - radius * radius;
    float t0, t1;

    if (!solveQuadratic(a, h, c, t0, t1)) return false;
    if (!ray_t.surrounds(t0)) t0 = t1; // if (!r.in_inclusive_bound(t0))
    if (!ray_t.surrounds(t0)) return false;
    
    // than it really hits
    hit_record.t = t0;
    hit_record.point = r.at(t0);
    Vec3f outward_normal = (hit_record.point - current_center) / radius;
    hit_record.set_face_normal(r, outward_normal);
    calculateUV(outward_normal, hit_record.u, hit_record.v);
    hit_record.material = material;

    return true;
}

/**
 * Get PDF value from Ray(origin, direction) when sampling the sphere
 * 
 * @note: given a sampling ray, get its sampling pdf value
 * - if the ray does not hit the sphere, pdf = 0
 * - if the ray hits the sphere, pdf = 1 / solid_angle
 */
float Sphere::getPDFValue(const Point3f& origin, const Vec3f& direction) const {
    HitRecord record;
    if (!this->hit(Ray(origin, direction), Interval(epsilon, infinity), record)) return 0;

    float distance_squared = (origin - moving_center.at(0)).length_squared();
    float cos_theta_max = std::sqrt(1 - radius * radius / distance_squared);
    float solid_angle = 2 * pi * (1 - cos_theta_max);
    return 1 / solid_angle;
}

/**
 * Generate a random direction from origin under the distribution of the sphere
 * @param origin: origin point outside the sphere
 * @note: given an origin point outside the sphere, uniformly sample a direction 
 *        that hits the sphere. use random_to_sphere() in local coordinates and
 *        transform to world coordinates using ONB.
 *  => sample uniformly in the cone formed by the sphere viewed from the origin.
 */
Vec3f Sphere::random(const Point3f& origin) const {
    Vec3f direction = moving_center.at(0) - origin;  // moving_center.at(0) = center
    float distance_squared = direction.length_squared();
    ONB uvw(direction);
    return uvw.transform(random_to_sphere(radius, distance_squared));
}


/* ==================== Private ==================== */

AABB Sphere::calculateAABB(const Point3f& center, const float& radius) {
    Vec3f radius_vec(radius, radius, radius);
    return AABB(center - radius_vec, center + radius_vec);
}

/**
 * Calculate UV coordinates on the sphere
 *  
 * point: point on the unit sphere (radius = 1, center = (0, 0, 0))
 *  point = (x, y, z)
 * u, v: texture coordinates [0, 1]
 * Calculation:
 *     { y = -cos(theta)             { theta = arccos(-y)
 *   - { x = -sin(theta)cos(phi)  => { phi'  = atan2(z,-x) 
 *     { z =  sin(theta)sin(phi)             = phi - pi 
 * 
 *   - { u = phi / 2pi      unification
 *     { v = theta / pi
 */
void Sphere::calculateUV(const Point3f& point, float& u, float& v) {
    float theta = std::acos(-point.y());
    float phi   = std::atan2(-point.z(), point.x()) + pi;
    u = phi / (2 * pi);
    v = theta / pi;
}

/**
 * Generate a random direction to the sphere from the origin, assume z axis to the norm
 * 
 * @param radius: radius of the sample target sphere
 * @param distance_squared: squared distance between origin and sphere center
 * @note Inversion method. Assume the view cone is aligned with the z axis. we want to 
 *  sample uniformly to the sphere, so the distribution should be uniform in the surface 
 *  area of the sphere. dS = sin(θ) dθ dφ, we need map (x, y) to (θ, φ).
 *  - θ: angle from z axis, [0, theta_max]
 *  - φ: azimuth angle surround z axis, [0, 2pi]
 * @note Randomly select (x, y) to be (r1, r2) ~ U(0, 1). we get equations
 *  - cos theta = 1 + r2 (cos theta_max - 1)
 *  - phi = 2 pi r1
 */
Vec3f Sphere::random_to_sphere(float radius, float distance_squared) {
    float r1 = random_float(), r2 = random_float();
    float z = 1 + r2 * (std::sqrt(1 - radius * radius / distance_squared) - 1);

    float phi = 2 * pi * r1;
    float sin_theta = std::sqrt(1 - z * z);
    float x = sin_theta * std::cos(phi);
    float y = sin_theta * std::sin(phi);

    return Vec3f(x, y, z);
}