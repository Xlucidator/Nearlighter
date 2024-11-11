#ifndef SPHERE_H
#define SPHERE_H

#include "vec3f.h"
#include "shape.h"

class Sphere : public Shape {
private:
    Point3f center;
    float   radius;

public:
    Sphere():radius(1.0) {}
    Sphere(const Point3f& center, const float& r):center(center), radius(r) {}

    bool hit(const Ray& r, HitRecord& hit_record) const override;
};

#endif
