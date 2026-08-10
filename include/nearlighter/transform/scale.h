#ifndef NEARLIGHTER_TRANSFORM_SCALE_H
#define NEARLIGHTER_TRANSFORM_SCALE_H

#include <nearlighter/geometry/shape.h>

#include <memory>
#include <stdexcept>
#include <utility>

/** Uniform positive scale around the world origin. */
class Scale : public Shape {
public:
    Scale(std::shared_ptr<Shape> shape, float factor)
        : shape_(std::move(shape)), factor_(factor) {
        if (factor_ <= 0.0f) {
            throw std::invalid_argument("scale factor must be positive");
        }

        const AABB& source_box = shape_->getBoundingBox();
        bounding_box_ = AABB(
            Point3f(source_box.x.min, source_box.y.min, source_box.z.min) *
                factor_,
            Point3f(source_box.x.max, source_box.y.max, source_box.z.max) *
                factor_);
    }

    bool hit(const Ray& ray, Interval ray_t, HitRecord& hit_record,
             Sampler& sampler) const override {
        /* ----- Object-space intersection ----- */
        const Ray scaled_ray(ray.origin() / factor_,
                             ray.direction() / factor_, ray.time());
        if (!shape_->hit(scaled_ray, ray_t, hit_record, sampler)) {
            return false;
        }

        /* ----- World-space result ----- */
        // Positive uniform scaling preserves t, normal direction, and facing.
        hit_record.point *= factor_;
        return true;
    }

    const AABB& getBoundingBox() const override { return bounding_box_; }

    bool hasPDF() const override { return shape_->hasPDF(); }

    float getPDFValue(const Point3f& origin,
                      const Vec3f& direction) const override {
        return shape_->getPDFValue(origin / factor_, direction / factor_);
    }

    Vec3f random(const Point3f& origin, Sampler& sampler) const override {
        return factor_ * shape_->random(origin / factor_, sampler);
    }

private:
    std::shared_ptr<Shape> shape_;
    float factor_;
    AABB bounding_box_;
};

#endif  // NEARLIGHTER_TRANSFORM_SCALE_H
