#include <nearlighter/scene/object/instance.h>

#include <stdexcept>
#include <utility>

Instance::Instance(std::shared_ptr<const Intersectable> source,
                   Transform local_to_parent)
    : source_(std::move(source)),
      local_to_parent_(std::move(local_to_parent)) {
    if (!source_) throw std::invalid_argument("Instance requires a source");
    bounds_ = local_to_parent_.applyBounds(source_->getBoundingBox());
}

/**
 * @par Implementation
 * parent Ray -> source Ray -> source event -> parent event.
 * The inverse ray transform does not normalize direction, preserving t.
 * Both event types retain their material and source Primitive bindings.
 */
bool Instance::hit(const Ray& ray, Interval ray_t, HitRecord& record,
                   Sampler& sampler) const {
    if (local_to_parent_.isIdentity()) {
        return source_->hit(ray, ray_t, record, sampler);
    }

    const Ray local_ray = local_to_parent_.applyInverseRay(ray);
    if (!source_->hit(local_ray, ray_t, record, sampler)) return false;
    record.transform(local_to_parent_, ray);
    return true;
}
