#ifndef NEARLIGHTER_SCENE_LINEAR_AGGREGATE_H
#define NEARLIGHTER_SCENE_LINEAR_AGGREGATE_H

#include <nearlighter/scene/intersectable.h>

#include <cstddef>
#include <memory>
#include <vector>

/**
 * Sequential Intersectable Aggregate
 *
 * Stores shared Intersectable references in insertion order.
 * - Intersection uses a linear closest-hit search.
 * - Cached bounds contain the union of all current members.
 *
 * Member bounds must remain stable while stored in the aggregate.
 */
class LinearAggregate final : public Intersectable {
public:
    LinearAggregate() = default;

    /** Creates an aggregate containing one non-null entity. */
    explicit LinearAggregate(std::shared_ptr<const Intersectable> object);

    bool hit(const Ray& ray, Interval ray_t, HitRecord& record,
             Sampler& sampler) const override;

    const AABB& getBoundingBox() const override { return bounds_; }

    /** @name Aggregate Mutation
     * Maintains the cached bounds together with the owned reference array.
     * `add()` rejects null entities and requires stable entity bounds.
     * @{ */
    void clear();
    void add(std::shared_ptr<const Intersectable> object);
    /** @} */

    /** @name Aggregate Inspection
     * Exposes immutable insertion-order state without transferring ownership.
     * @{ */
    std::size_t size() const { return objects_.size(); }
    bool empty() const { return objects_.empty(); }
    const std::vector<std::shared_ptr<const Intersectable>>& objects() const {
        return objects_;
    }
    /** @} */

private:
    std::vector<std::shared_ptr<const Intersectable>> objects_;
    AABB bounds_;
};

#endif  // NEARLIGHTER_SCENE_LINEAR_AGGREGATE_H
