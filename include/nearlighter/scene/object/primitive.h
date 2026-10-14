#ifndef NEARLIGHTER_SCENE_OBJECT_PRIMITIVE_H
#define NEARLIGHTER_SCENE_OBJECT_PRIMITIVE_H

#include <nearlighter/scene/object/intersectable.h>
#include <nearlighter/shape/shape.h>
#include <nearlighter/geometry/transform.h>

#include <memory>

class Material;
class HitMethod;

/**
 * Bounded Surface or Volume Primitive
 *
 * Binds immutable Shape, Material, and HitMethod to a parent-space placement.
 * - Shape supplies intrinsic geometry and local surface sampling.
 * - Transform maps Shape-local geometry into the parent space.
 * - Material supplies either surface or volume properties.
 * - HitMethod locates events; shading waits until the closest event is selected.
 */
class Primitive final : public Intersectable {
public:
    /**
     * Primitive Construction
     *
     * Retains shared components and caches transformed bounds.
     *
     * @param shape Non-null geometry expressed in its local space.
     * @param material Non-null surface or volume properties.
     * @param local_to_parent Affine mapping from Shape space to parent space.
     * @param hit_method Optional method; omission selects the material family default.
     * @throws std::invalid_argument For null components or incompatible methods.
     */
    Primitive(std::shared_ptr<const Shape> shape,
              std::shared_ptr<const Material> material,
              Transform local_to_parent = {},
              std::shared_ptr<const HitMethod> hit_method = {});

    bool hit(const Ray& ray, Interval ray_t, HitRecord& record,
             Sampler& sampler) const override;

    const AABB& getBoundingBox() const override { return bounds_; }

    /** @name Placed Surface Sampling
     * Converts the Shape distribution into the parent solid-angle space.
     * @{ */
    bool hasPDF() const;
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

    /** New placement sharing Shape, Material, and HitMethod, with parent_to_world applied last. */
    Primitive transformed(const Transform& parent_to_world) const;

private:
    std::shared_ptr<const Shape> shape_;
    std::shared_ptr<const Material> material_;
    std::shared_ptr<const HitMethod> hit_method_;
    Transform local_to_parent_;
    AABB bounds_;
};

#endif  // NEARLIGHTER_SCENE_OBJECT_PRIMITIVE_H
