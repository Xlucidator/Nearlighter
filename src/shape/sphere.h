#ifndef SPHERE_H
#define SPHERE_H

#include "common.h"
#include "shape.h"

class Sphere : public Shape {
public:
    Sphere():radius(1.0) {}
    // Stationary Object
    Sphere(const Point3f& center, const float& radius, shared_ptr<Material> material)
        : center(center), radius(std::fmax(0, radius)), material(material), 
          moving_center(center, Vec3f(0, 0, 0)) {
        bounding_box = Sphere::calculateAABB(center, radius);
    }
    // Moving Object
    Sphere(const Point3f& center_start, const Point3f& center_end, const float& radius, shared_ptr<Material> material)
        : center(center_start), radius(std::fmax(0, radius)), material(material), 
          moving_center(center_start, center_end - center_start) {
        AABB box_start = Sphere::calculateAABB(center_start, radius);
        AABB box_end   = Sphere::calculateAABB(center_end, radius);
        bounding_box = AABB(box_start, box_end);
    }

    bool hit(const Ray& r, Interval ray_t, HitRecord& hit_record) const override;
    const AABB& getBoundingBox() const override { return bounding_box; }

private:
    Point3f center;
    float   radius;
    shared_ptr<Material> material;
    Ray     moving_center;

    AABB    bounding_box;
    static AABB calculateAABB(const Point3f& center, const float& radius);
    static void calculateUV(const Point3f& point, float& u, float& v);
};

#endif
