#include "quad.h"

#include "utils/constant.h"
#include "shapelist.h"

/** 
 * Hit a Quad Primitive
 * 
 * Ray: o + t \vec{d}
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

/**
 * Get PDF value from Ray(origin, direction) when sampling the quad
 */
float Quad::getPDFValue(const Point3f& origin, const Vec3f& direction) const {
    HitRecord record; 
    if (!this->hit(Ray(origin, direction), Interval(epsilon, infinity), record)) return 0;
    float distance_squared = record.t * record.t * direction.length_squared();
    float cosine = dot(direction, record.normal) / direction.length();
    cosine = std::fabs(cosine); // TODO: dealing with backward faces
    return distance_squared / (cosine * area);
} 

/**
 * Generate a random direction from origin under the distribution of the quad
 */
Vec3f Quad::random(const Point3f& origin) const {
    Vec3f end = p0 + u * random_float() + v * random_float();
    return end - origin;
}


/* ==================== Private ==================== */

/**
 * Calculate AABB of the quad
 * 
 * union of two diagonal lines
 *         p0+v_____p0+u+v
 *         /\      /
 *        v  \    /
 *       /    \  /
 *      /_ u __\/
 *   p0        p0+u
 */
AABB Quad::calculateAABB(const Point3f& p0, const Vec3f& u, const Vec3f& v) {
    return AABB(AABB(p0, p0 + u + v), AABB(p0 + u, p0 + v));
}

/* Common Method */

/** 
 * Create Box Object with Quad Primitive
 * 
 *  a, b is two opposite vertices of the box
 */
shared_ptr<Shape> box(const Point3f& a, const Point3f& b, shared_ptr<Material> material) {
    auto box_sides = make_shared<ShapeList>();
    
    Point3f p_min = Point3f(std::fmin(a.x(), b.x()), std::fmin(a.y(), b.y()), std::fmin(a.z(), b.z()));
    Point3f p_max = Point3f(std::fmax(a.x(), b.x()), std::fmax(a.y(), b.y()), std::fmax(a.z(), b.z()));
    Vec3f vec_x = Vec3f(p_max.x() - p_min.x(), 0, 0);
    Vec3f vec_y = Vec3f(0, p_max.y() - p_min.y(), 0);
    Vec3f vec_z = Vec3f(0, 0, p_max.z() - p_min.z());

    box_sides->add(make_shared<Quad>(p_max, -vec_x, -vec_y, material)); // front
    box_sides->add(make_shared<Quad>(p_min,  vec_y,  vec_x, material)); // back
    box_sides->add(make_shared<Quad>(p_max, -vec_y, -vec_z, material)); // right
    box_sides->add(make_shared<Quad>(p_min,  vec_z,  vec_y, material)); // left
    box_sides->add(make_shared<Quad>(p_max, -vec_z, -vec_x, material)); // top
    box_sides->add(make_shared<Quad>(p_min,  vec_x,  vec_z, material)); // bottom

    return box_sides;
}