#ifndef SPHERE_H
#define SPHERE_H

#include "common.h"
#include "shape.h"

class Sphere : public Shape {
private:
    Point3f center;
    float   radius;
    shared_ptr<Material> material;

public:
    Sphere():radius(1.0) {}
    Sphere(const Point3f& center, const float& radius, shared_ptr<Material> material)
        : center(center), radius(std::fmax(0.0, radius)), material(material) {}

    bool hit(const Ray& r, Interval ray_t, HitRecord& hit_record) const override;
};

#endif
