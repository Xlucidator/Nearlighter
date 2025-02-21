#ifndef SHAPE_H
#define SHAPE_H

#include "common.h"
#include "bvh/aabb.h"

class Material;

struct HitRecord {
    Point3f point;
    Vec3f normal;
    shared_ptr<Material> material;
    float t;
    bool front_face;

    // We decide, that normal should always point against the ray
    // Assume: outward_normal is unit vector
    void set_face_normal(const Ray& r, const Vec3f& outward_normal) {
        front_face = dot(r.direction(), outward_normal) < 0; // true : if normal's direction point against the ray
        // front_face: means face to face
        normal = front_face ? outward_normal : -outward_normal;
    }
};

class Shape {
public:
    virtual ~Shape() = default;
    virtual bool hit(const Ray& r, Interval ray_t, HitRecord& hit_record) const = 0;
    virtual const AABB& getBoundingBox() const = 0;

    /* Debug */
    virtual void printNode(int level) const {}
};

#include "sphere.h"

#endif
