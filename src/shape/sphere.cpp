#include "sphere.h"

#include <iostream>

/* Ray: o + t \vec{d}, Sphere: (p - c)^2 = r^2
 * Hit:
 *  => (o + t \vec{d} - c)^2 = r^2
 *  => \vec{d}^2 t^2 + 2 \vec{d} (o-c) t + (o-c)^2 - r^2 = 0
 *  => Solving the Quadratic equation: a = \vec{d}^2, b = 2\vec{d}(o-c), c = (o-c)^2-r^2
 *  => Solution t should in the ray bound
 */
bool Sphere::hit(const Ray& r, Interval ray_t, HitRecord& hit_record) const {
    Vec3f co = r.origin() - center;
    
    float a = dot(r.direction(), r.direction());
    float b = 2 * dot(r.direction(), co);
    float c = dot(co, co) - radius * radius;
    float t0, t1;

    if (!solveQuadratic(a, b, c, t0, t1)) return false;
    if (!ray_t.surrounds(t0)) t0 = t1; // if (!r.in_inclusive_bound(t0))
    if (!ray_t.surrounds(t0)) return false;
    
    // than it really hits
    hit_record.t = t0;
    hit_record.point = r.at(t0);
    Vec3f outward_normal = (hit_record.point - center) / radius;
    hit_record.set_face_normal(r, outward_normal);
    hit_record.material = material;

    return true;
}