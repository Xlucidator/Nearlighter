#ifndef NEARLIGHTER_ACCEL_BVH_H
#define NEARLIGHTER_ACCEL_BVH_H

#include <nearlighter/scene/intersectable.h>

#include <memory>
#include <vector>

class LinearAggregate;

/**
 * Binary hierarchy over immutable Intersectable bounds in one coordinate space.
 *
 * Construction partitions a private object array by bounding-box centroid.
 * - Leaf    : stores exactly one object
 * - Non-leaf: caches the union of all descendant bounds.
 *
 * Traversal clips the second child query to the closest first-child hit
 * while preserving the standard Intersectable result.
 */
class BVH final : public Intersectable {
public:
    /** Delegates construction from an aggregate snapshot. */
    explicit BVH(const LinearAggregate& aggregate);

    /**
     * Builds from an owned pointer array that may be reordered internally.
     *
     * @param objects Non-empty array of non-null bounded objects.
     * @throws std::invalid_argument when objects is empty or contains null.
     */
    explicit BVH(
        std::vector<std::shared_ptr<const Intersectable>> objects);

    ~BVH() override;

    // Disable copying; Allow exclusive node ownership to be moved.
    BVH(const BVH&) = delete;
    BVH& operator=(const BVH&) = delete;
    BVH(BVH&&) noexcept;
    BVH& operator=(BVH&&) noexcept;

    /** Implements closest-hit traversal over the hierarchy. */
    bool hit(const Ray& ray, Interval ray_t, HitRecord& record,
             Sampler& sampler) const override;

    /** Returns the cached union of every leaf's bounds. */
    const AABB& getBoundingBox() const override { return bounds_; }

    /**
     * Leaf Object Collection
     *
     * Allocates an object-reference array for scene inspection.
     * - Includes one entry per leaf, preserving repeated object references.
     * - Uses left-to-right tree order, not the original input order.
     * - Keeps nested aggregates and instances intact.
     */
    std::vector<std::shared_ptr<const Intersectable>> collectObjects() const;

private:
    struct Node;

    std::unique_ptr<Node> root_;
    AABB bounds_;
};

#endif  // NEARLIGHTER_ACCEL_BVH_H
