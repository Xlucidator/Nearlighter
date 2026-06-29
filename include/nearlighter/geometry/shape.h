#ifndef SHAPE_H
#define SHAPE_H

#include <nearlighter/nearlighter.h>
#include <nearlighter/geometry/aabb.h>

class Material;

struct HitRecord {
    Point3f point;
    Vec3f normal;
    shared_ptr<Material> material;
    float t;
    float u, v;
    bool front_face;

    void set_face_normal(const Ray& r, const Vec3f& outward_normal) {
        // Normals are always oriented against the incident ray.
        // Assumption: outward_normal is already unit length.
        front_face = dot(r.direction(), outward_normal) < 0;
        normal = front_face ? outward_normal : -outward_normal;
    }
};

class Shape {
public:
    virtual ~Shape() = default;
    virtual bool hit(const Ray& r, Interval ray_t, HitRecord& hit_record) const = 0;
    virtual const AABB& getBoundingBox() const = 0;

    virtual bool hasPDF() const { return false; }
    virtual float getPDFValue(const Point3f& origin, const Vec3f& direction) const { return 0.0f; }
    virtual Vec3f random(const Point3f& origin) const { return Vec3f(1, 0, 0); }

    /* Debug */
    virtual void printNode(int level) const {}
};

#endif
