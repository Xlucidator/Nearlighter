#ifndef TRANSLATE_H
#define TRANSLATE_H

#include "shape/shape.h"

class Translate : public Shape {
public:
    Translate(shared_ptr<Shape> shape, Vec3f offset) : shape(shape), offset(offset) {
        bbox = shape->getBoundingBox() + offset;
    }

    bool hit(const Ray& ray, Interval ray_t, HitRecord& record) const override {
        /* Transform the ray from world space to object space */
        // Move the ray in the opposite direction of the translation
        Ray translated_ray(ray.origin() - offset, ray.direction());

        // Hit object with translated ray
        if (!shape->hit(translated_ray, ray_t, record))
            return false;
        
        /* Transform the intersection from object space back to world spac */
        // Move the hit point back to the original position
        record.point += offset;
        return true;
    }

    const AABB& getBoundingBox() const override { return bbox; }

private:
    shared_ptr<Shape> shape;
    Vec3f offset;
    AABB bbox;
};





#endif // TRANSLATE_H