#ifndef SHAPELIST_H
#define SHAPELIST_H

#include "common.h"
#include "shape.h"
#include "bvh/aabb.h"

#include <vector>

class ShapeList : public Shape {
public:
    std::vector<shared_ptr<Shape>> objects;

    ShapeList() {}
    ShapeList(shared_ptr<Shape> object) { add(object); }

    bool hit(const Ray& r, Interval ray_t, HitRecord& closest_hitrec) const override {
        HitRecord tmp_rec;
        bool hit_anything = false;
        float closest_t = ray_t.max;  // r.tmax : whether implement 'interval' in ray or seperate out

        for (const auto& object : objects) {
            if (object->hit(r, Interval(ray_t.min, closest_t), tmp_rec)) {
                hit_anything = true;
                closest_t = tmp_rec.t;
                closest_hitrec = tmp_rec;
            }
        }

        return hit_anything;
    }
    const AABB& getBoundingBox() const override { return bbox; }

    float getPDFValue(const Point3f& origin, const Vec3f& direction) const override { 
        float weight = 1.0 / objects.size();
        float sum = 0.0;
        for (const auto& object : objects)
            sum += weight * object->getPDFValue(origin, direction);
        return sum;
    }

    Vec3f random(const Point3f& origin) const override { 
        return objects[random_int(0, objects.size()-1)]->random(origin); 
    }

    void clear() { objects.clear(); }
    void add(shared_ptr<Shape> object) { 
        objects.push_back(object);
        bbox.uunion(object->getBoundingBox());
    }
    size_t size() const { return objects.size(); }
    bool emplty() const { return objects.empty(); }

private:
    AABB bbox;
};

#endif
