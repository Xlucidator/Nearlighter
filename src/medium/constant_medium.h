#ifndef CONSTANT_MEDIUM_H
#define CONSTANT_MEDIUM_H

#include "shape/shape.h"
#include "texture/texture.h"

// #define MEDIUM_DEBUG

class ConstantMedium : public Shape {
public:
    // ConstantMedium(shared_ptr<Shape> boundary, float density, shared_ptr<Material> phase_function); // Disable other Material
    ConstantMedium(shared_ptr<Shape> boundary, float density, shared_ptr<Texture> tex);
    ConstantMedium(shared_ptr<Shape> boundary, float density, const Color& albedo);

    bool hit(const Ray& r, Interval ray_t, HitRecord& record) const override;
    const AABB& getBoundingBox() const override { return boundary->getBoundingBox(); }

private:
    shared_ptr<Shape> boundary;
    float density, neg_inv_density;
    shared_ptr<Material> phase_function;
};

#endif // CONSTANT_MEDIUM_H
