#ifndef SPHERE_H
#define SPHERE_H

#include <nearlighter/nearlighter.h>
#include <nearlighter/geometry/shape.h>

class Sphere : public Shape {
public:
    Sphere();
    // Stationary Object
    Sphere(const Point3f& center, const float& radius, shared_ptr<Material> material);
    // Moving Object
    Sphere(const Point3f& center_start, const Point3f& center_end, const float& radius, shared_ptr<Material> material);

    bool hit(const Ray& r, Interval ray_t, HitRecord& hit_record) const override;
    const AABB& getBoundingBox() const override { return bounding_box; }

    bool hasPDF() const override { return true; }
    float getPDFValue(const Point3f& origin, const Vec3f& direction) const override;
    Vec3f random(const Point3f& origin) const override;

private:
    Point3f center;
    float   radius;
    shared_ptr<Material> material;
    Ray     moving_center;

    AABB    bounding_box;
    static AABB calculateAABB(const Point3f& center, const float& radius);
    static void calculateUV(const Point3f& point, float& u, float& v);
    static Vec3f random_to_sphere(float radius, float distance_squared);
};

#endif
