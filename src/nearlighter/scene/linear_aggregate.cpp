#include <nearlighter/scene/linear_aggregate.h>

#include <stdexcept>
#include <utility>

LinearAggregate::LinearAggregate(
    std::shared_ptr<const Intersectable> object) {
    add(std::move(object));
}

/**
 * @par Implementation
 * Traverse members in insertion order.
 *
 * After each hit:
 * - Copy its record as the current closest result.
 * - Restrict subsequent queries to `[ray_t.min, candidate.t]`.
 */
bool LinearAggregate::hit(const Ray& ray, Interval ray_t, HitRecord& record,
                          Sampler& sampler) const {
    HitRecord candidate;
    bool hit_anything = false;
    float closest_t = ray_t.max;
    for (const auto& object : objects_) {
        if (object->hit(ray, Interval(ray_t.min, closest_t), candidate,
                        sampler)) {
            hit_anything = true;
            closest_t = candidate.t;
            record = candidate;
        }
    }
    return hit_anything;
}

void LinearAggregate::clear() {
    objects_.clear();
    bounds_ = AABB();
}

void LinearAggregate::add(std::shared_ptr<const Intersectable> object) {
    if (!object) {
        throw std::invalid_argument("LinearAggregate cannot contain null");
    }
    bounds_.uunion(object->getBoundingBox());
    objects_.push_back(std::move(object));
}
