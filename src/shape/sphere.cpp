#include "sphere.h"
#include "utils/math.h"

/* Ray: o + t \vec{d}, Sphere: (p - c)^2 = r^2
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

AABB Sphere::calculateAABB(const Point3f& center, const float& radius) {
    Vec3f radius_vec(radius, radius, radius);
    return AABB(center - radius_vec, center + radius_vec);
}

/* point: point on the unit sphere (radius = 1, center = (0, 0, 0))
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