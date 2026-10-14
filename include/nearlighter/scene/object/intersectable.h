#ifndef NEARLIGHTER_SCENE_OBJECT_INTERSECTABLE_H
#define NEARLIGHTER_SCENE_OBJECT_INTERSECTABLE_H

#include <nearlighter/base/interval.h>
#include <nearlighter/base/ray.h>
#include <nearlighter/geometry/aabb.h>
#include <nearlighter/render/hit/hit_record.h>

class Sampler;

/**
 * Bounded Render Entity
 *
 * Shared query contract for leaves, aggregates, and transformed instances.
 * - Rays, interactions, and bounds use the caller's current coordinate space.
 * - A top-level query uses world space.
 * - Instance queries source space and maps the result back to parent space.
 * - Implementations locate events; the integrator determines transport.
 */
class Intersectable {
public:
    virtual ~Intersectable() = default;

    /**
     * Closest Surface or Volume Event
     *
     * @param ray Ray in the current coordinate space.
     * @param ray_t Accepted range of ray parameters.
     * @param record Receives the nearest complete event; unchanged on a miss.
     * @param sampler Random stream for stochastic events; unused by surfaces.
     * @return `true` when an accepted interaction exists.
     */
    virtual bool hit(const Ray& ray, Interval ray_t, HitRecord& record,
                     Sampler& sampler) const = 0;

    /**
     * Entity Bounds
     *
     * @return Caller-space bounds; the reference lasts for the entity lifetime.
     */
    virtual const AABB& getBoundingBox() const = 0;
};

#endif  // NEARLIGHTER_SCENE_OBJECT_INTERSECTABLE_H
