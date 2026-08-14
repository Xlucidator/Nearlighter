#ifndef NEARLIGHTER_SCENE_INSTANCE_H
#define NEARLIGHTER_SCENE_INSTANCE_H

#include <nearlighter/scene/intersectable.h>
#include <nearlighter/geometry/transform.h>

#include <memory>

/**
 * Transformed Intersectable Instance
 *
 * Places one shared Intersectable subtree in a parent coordinate space.
 * - Source storage and acceleration are reused without rebuilding.
 * - Material, UV coordinates, and ray parameter pass through unchanged.
 * - Position, normals, facing, and bounds map to parent space.
 *
 * Source bounds must remain stable because parent-space bounds are cached.
 */
class Instance final : public Intersectable {
public:
    /**
     * Instance Construction
     *
     * @param source Non-null entity or aggregate expressed in source space.
     * @param local_to_parent Affine mapping from source to parent space.
     * @throws std::invalid_argument if `source` is null.
     */
    Instance(std::shared_ptr<const Intersectable> source,
             Transform local_to_parent);

    bool hit(const Ray& ray, Interval ray_t, HitRecord& record,
             Sampler& sampler) const override;

    const AABB& getBoundingBox() const override { return bounds_; }

private:
    std::shared_ptr<const Intersectable> source_;
    Transform local_to_parent_;
    AABB bounds_;
};

#endif  // NEARLIGHTER_SCENE_INSTANCE_H
