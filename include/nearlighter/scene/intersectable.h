#ifndef NEARLIGHTER_SCENE_INTERSECTABLE_H
#define NEARLIGHTER_SCENE_INTERSECTABLE_H

#include <nearlighter/base/interval.h>
#include <nearlighter/base/ray.h>
#include <nearlighter/geometry/aabb.h>

class Material;
class Primitive;
class Sampler;

/** Distinguishes physical surface hits from stochastic medium events. */
enum class InteractionKind {
    Surface,
    Medium,
};

/**
 * Render Interaction
 *
 * Stores one complete interaction in the coordinate space of the queried
 * Intersectable.
 * - Surface events store unit normals facing against the ray; `normal` may
 *   contain a shading normal compatible with `geometric_normal`.
 * - Surface `front_face` records whether the ray reached the outward side.
 * - Volume events have no surface orientation or UV parameterization and use
 *   neutral placeholders for those fields.
 * - `material` is non-owning and remains valid while the hit object is alive.
 */
struct HitRecord {
    Point3f point;
    Vec3f geometric_normal;
    Vec3f normal;
    const Material* material = nullptr;
    const Primitive* primitive = nullptr;
    float t = 0.0f;
    float u = 0.0f;
    float v = 0.0f;
    bool front_face = false;
    InteractionKind kind = InteractionKind::Surface;

    /**
     * Surface Normal Orientation
     *
     * Derives `front_face` from the geometric normal, then stores both normals
     * against the incident direction.
     *
     * @param ray Incident ray in the current coordinate space.
     * @param outward_geometric Unit outward geometric normal in the same space.
     * @param outward_shading Unit outward shading normal in the same
     * hemisphere and coordinate space as `outward_geometric`.
     */
    void setFaceNormals(const Ray& ray, const Vec3f& outward_geometric,
                        const Vec3f& outward_shading) {
        front_face = dot(ray.direction(), outward_geometric) < 0.0f;
        geometric_normal = front_face ? outward_geometric
                                      : -outward_geometric;
        normal = front_face ? outward_shading : -outward_shading;
    }
};

/**
 * Bounded Render Entity
 *
 * Defines the common query contract for renderable leaves, aggregates,
 * transformed instances, and stochastic media.
 * - Rays, interactions, and bounds use the caller's current coordinate space.
 * - A top-level query uses world space; Instance recursively introduces a
 *   source space and converts the result back to its parent space.
 * - Implementations return the closest accepted interaction without applying
 *   renderer integration policy.
 */
class Intersectable {
public:
    virtual ~Intersectable() = default;

    /**
     * Closest Render Interaction
     *
     * @param ray Ray in the current coordinate space.
     * @param ray_t Accepted range of ray parameters.
     * @param record Receives the closest complete interaction on success; it
     * remains unchanged on a miss.
     * @param sampler Random stream available to stochastic implementations;
     * deterministic surfaces do not consume it.
     * @return `true` when an accepted interaction exists.
     */
    virtual bool hit(const Ray& ray, Interval ray_t, HitRecord& record,
                     Sampler& sampler) const = 0;

    /**
     * Entity Bounds
     *
     * @return Axis-aligned bounds in the current coordinate space. The
     * reference remains valid for the entity's lifetime.
     */
    virtual const AABB& getBoundingBox() const = 0;
};

#endif  // NEARLIGHTER_SCENE_INTERSECTABLE_H
