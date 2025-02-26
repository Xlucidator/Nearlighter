#include "quad.h"

#include "utils/constant.h"

/* Ray: o + t \vec{d}
 * Quad: Plane Ax + By + Cz = D
 *  - (A, B, C) = \vec{n} = cross(u, v)
 *  - A(x-x0) + B(y-y0) + C(z-z0) = 0 <=> D = Ax0 + By0 + Cz0 = p0 \vec{n}
 * Hit:
 *  1. find the plane of the quad √
 *  2. solving the intersection with the ray
 *      => ((o + t \vec{d}) - p0) \vec{n} = 0
 *      =>  (o + t \vec{d}) \vec{n} = p0 \vec{n} = D
 *      =>  o \vec{n} + t \vec{d} \vec{n} = D
 *      =>  t = (D - o \vec{n}) / (\vec{d} \vec{n})
 *  3. check if the intersection P is inside the quad
 *      => measure the coordinates (a, b) of the intersction point with the base vector u, v
 *      => (a, b) means \vec{p} = P - p0 = a\vec{u} + b\vec{v}; P is inside the quad <=> 0 <= a <= 1, 0 <= b <= 1
 *      => *we can find a, b by cross product calculations:
 *          - cross(u, p) = cross(u, a\vec{u} + b\vec{v}) = a cross(u, u) + b cross(u, v) = b cross(u, v)
 *          => cross(u, p) = b cross(u, v), let \vec{n} = cross(u, v)
 *          => b = (n cross(u, p)) / (n cross(u, v)) ; [wrong] b = |cross(u, p)| / |cross(u, v)|, it will not tackle the sign(direction) of the vector
 *          - similarly
 *          => a = (n cross(v, p)) / (n cross(v, u)) = (n cross(p, v)) / (n cross(u, v))
 *      => we can previously cache the variable \vec{w} = n / (n cross(u, v)) = n / (n n)
 */
bool Quad::hit(const Ray& r, Interval ray_t, HitRecord& hit_record) const {
    float d_n = dot(r.direction(), normal);
    // std::cout << "start hit, d_n " << std::fabs(d_n) << std::endl;
    if (std::fabs(d_n) < epsilon) return false; // which means ray is nearly parallel to the quad plane

    // std::cout << "hit plane" << std::endl;

    // get intersection point
    float t = (D - dot(r.origin(), normal)) / d_n;
    if (!ray_t.contains(t)) return false;

    // std::cout << "within ray bound" << std::endl;

    // check intersection point
    Point3f P = r.at(t); // intersection point of the plane
    Vec3f p = P - p0;    // p0 -> P
    float b = dot(w, cross(u, p)), a = dot(w, cross(p, v));
    if (!Interval::unit.contains(a) || !Interval::unit.contains(b)) return false;

    // std::cout << "in quad bound" << std::endl;

    // than it really hits
    hit_record.t = t;
    hit_record.point = r.at(t);
    hit_record.set_face_normal(r, normal);
    hit_record.u = a, hit_record.v = b;
    hit_record.material = material;

    return true;
}


/* Private */

/* Calculate AABB of the quad: union of two diagonal lines
 *         p0+v__________p0+u+v
 *          /\         /
 *        v   \      /
 *       /     \   /
 *   p0/___ u __\/p0+u
 */
AABB Quad::calculateAABB(const Point3f& p0, const Vec3f& u, const Vec3f& v) {
    return AABB(AABB(p0, p0 + u + v), AABB(p0 + u, p0 + v));
}