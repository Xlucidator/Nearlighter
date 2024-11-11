#ifndef SHAPELIST_H
#define SHAPELIST_H

#include "shape.h"

#include <memory>
#include <vector>

using std::make_shared;
using std::shared_ptr;

class ShapeList : public Shape {
  public:
    std::vector<shared_ptr<Shape>> objects;

    ShapeList() {}
    ShapeList(shared_ptr<Shape> object) { add(object); }

    bool hit(const Ray& r, HitRecord& closest_hitrec) const override {
        HitRecord tmp_rec;
        bool hit_anything = false;
        float closest_t = r.tmax;

        for (const auto& object : objects) {
            if (object->hit(r, tmp_rec) && tmp_rec.t < closest_t) {
                hit_anything = true;
                closest_t = tmp_rec.t;
                closest_hitrec = tmp_rec;
            }
        }

        return hit_anything;
    }

    void clear() { objects.clear(); }
    void add(shared_ptr<Shape> object) { objects.push_back(object); }
};

#endif
