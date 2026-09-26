#ifndef NEARLIGHTER_SCENE_PRIMITIVE_H
#define NEARLIGHTER_SCENE_PRIMITIVE_H

#include <nearlighter/scene/intersectable.h>
#include <nearlighter/shape/shape.h>
#include <nearlighter/geometry/transform.h>

#include <memory>

/**
 * Renderable Surface Primitive
 *
 * Binds one immutable local Shape to a Material and a parent-space placement.
 * - Shape supplies intrinsic geometry and local surface sampling.
 * - Transform maps Shape-local geometry into the parent space.
 * - Material completes each accepted surface interaction.
 */
class Primitive final : public Intersectable {
public:
    /**
     * Primitive Construction
     *
     * Retains immutable geometry and material by shared ownership and caches
     * the transformed bounds.
     *
     * @param shape Non-null geometry expressed in its local space.
     * @param material Non-null response bound to every surface interaction.
     * @param local_to_parent Affine mapping from Shape space to parent space.
     * @throws std::invalid_argument if `shape` or `material` is null.
     */
    Primitive(std::shared_ptr<const Shape> shape,
              std::shared_ptr<const Material> material,
              Transform local_to_parent = {});

    bool hit(const Ray& ray, Interval ray_t, HitRecord& record,
             Sampler& sampler) const override;

    const AABB& getBoundingBox() const override { return bounds_; }

    /** @name Placed Surface Sampling
     * Converts the Shape distribution into the parent solid-angle space.
     * @{ */
    bool hasPDF() const { return shape_->hasPDF(); }
    float getPDFValue(const Point3f& origin,
                      const Vec3f& direction) const;
    Vec3f random(const Point3f& origin, Sampler& sampler) const;
    /** @} */

    /** @name Bound Components
     * Returns non-owning references to the immutable bound components.
     * @{ */
    const Shape& shape() const { return *shape_; }
    const Material& material() const { return *material_; }
    const Transform& localToParent() const { return local_to_parent_; }
    /** @} */

    /** New placement sharing Shape/Material, with parent_to_world applied last. */
    Primitive transformed(const Transform& parent_to_world) const;

private:
    std::shared_ptr<const Shape> shape_;
    std::shared_ptr<const Material> material_;
    Transform local_to_parent_;
    AABB bounds_;
};

#endif  // NEARLIGHTER_SCENE_PRIMITIVE_H
