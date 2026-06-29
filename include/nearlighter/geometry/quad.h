#ifndef QUAD_H
#define QUAD_H

#include <nearlighter/nearlighter.h>
#include <nearlighter/geometry/shape.h>

class Quad : public Shape {
public:
    Quad(const Point3f& p0, const Vec3f& u, const Vec3f& v, shared_ptr<Material> material);

    bool hit(const Ray& r, Interval ray_t, HitRecord& hit_record) const override;
    const AABB& getBoundingBox() const override { return bounding_box; }

    bool hasPDF() const override { return true; }
    float getPDFValue(const Point3f& origin, const Vec3f& direction) const override; 
    Vec3f random(const Point3f& origin) const override;

private:
    // Described in Vector form
    Point3f p0;
    Vec3f   u, v;
    // Equation of the quad plane: Ax + By + Cz = D
    Vec3f normal;   // normal = (A, B, C)
    float D;        // D
    // Cached temporary variable
    Vec3f w;        // w = n / (n n) 
    // Properties
    shared_ptr<Material> material;
    float area;

    AABB    bounding_box;
    static AABB calculateAABB(const Point3f& p0, const Vec3f& u, const Vec3f& v);
};

/* Create Box */
shared_ptr<Shape> box(const Point3f& a, const Point3f& b, shared_ptr<Material> material);


#endif // QUAD_H
