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
          moving_center(center, Vec3f(0, 0, 0)) {}
    // Moving Object
    Sphere(const Point3f& center_start, const Point3f& center_end, const float& radius, shared_ptr<Material> material)
        : center(center_start), radius(std::fmax(0, radius)), material(material), 
          moving_center(center_start, center_end - center_start) {}

    bool hit(const Ray& r, Interval ray_t, HitRecord& hit_record) const override;

private:
    Point3f center;
    float   radius;
    shared_ptr<Material> material;
    
    Ray     moving_center;
};

#endif
