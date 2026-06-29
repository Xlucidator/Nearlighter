#ifndef SHAPELIST_H
#define SHAPELIST_H

#include <nearlighter/nearlighter.h>
#include <nearlighter/geometry/shape.h>
#include <nearlighter/geometry/aabb.h>

#include <vector>

class ShapeList : public Shape {
public:
    std::vector<shared_ptr<Shape>> objects;

    ShapeList();
    ShapeList(shared_ptr<Shape> object);

    bool hit(const Ray& r, Interval ray_t, HitRecord& closest_hitrec) const override;
    const AABB& getBoundingBox() const override { return bbox; }

    bool hasPDF() const override;
    float getPDFValue(const Point3f& origin, const Vec3f& direction) const override;
    Vec3f random(const Point3f& origin) const override;

    void clear();
    void add(shared_ptr<Shape> object);
    size_t size() const { return objects.size(); }
    bool emplty() const { return objects.empty(); }

private:
    AABB bbox;
};

#endif
