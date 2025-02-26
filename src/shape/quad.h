#ifndef QUAD_H
#define QUAD_H

#include "common.h"
#include "shape.h"

class Quad : public Shape {
public:
    Quad(const Point3f& p0, const Vec3f& u, const Vec3f& v, shared_ptr<Material> material)
        : p0(p0), u(u), v(v), material(material) {
        Vec3f n = cross(u, v);
        normal = unit_vector(n);
        D = dot(normal, p0);
        w = n / dot(n, n);
        bounding_box = Quad::calculateAABB(p0, u, v);
    }

    bool hit(const Ray& r, Interval ray_t, HitRecord& hit_record) const override;
    const AABB& getBoundingBox() const override { return bounding_box; }

private:
    // Described in Vector form
    Point3f p0;
    Vec3f   u, v;
    // Equaation of the quad plane: Ax + By + Cz = D
    Vec3f normal;   // normal = (A, B, C)
    float D;        // D
    // Cached temporary variable
    Vec3f w;        // w = n / (n n) 
    // Properties
    shared_ptr<Material> material;

    AABB    bounding_box;
    static AABB calculateAABB(const Point3f& p0, const Vec3f& u, const Vec3f& v);
};
#endif // QUAD_H
