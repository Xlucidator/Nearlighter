#ifndef CONSTANT_MEDIUM_H
#define CONSTANT_MEDIUM_H

#include <nearlighter/geometry/shape.h>
#include <nearlighter/texture/texture.h>

#include <memory>

// #define MEDIUM_DEBUG

class ConstantMedium : public Shape {
public:
    // ConstantMedium(std::shared_ptr<Shape> boundary, float density, std::shared_ptr<Material> phase_function); // Disable other Material
    ConstantMedium(std::shared_ptr<Shape> boundary, float density, std::shared_ptr<Texture> tex);
    ConstantMedium(std::shared_ptr<Shape> boundary, float density, const Color& albedo);

    bool hit(const Ray& r, Interval ray_t, HitRecord& record,
             Sampler& sampler) const override;
    const AABB& getBoundingBox() const override { return boundary->getBoundingBox(); }

private:
    std::shared_ptr<Shape> boundary;
    float density, neg_inv_density;
    std::shared_ptr<Material> phase_function;
};

#endif // CONSTANT_MEDIUM_H
