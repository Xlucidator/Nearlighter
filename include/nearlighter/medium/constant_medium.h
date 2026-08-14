#ifndef NEARLIGHTER_MEDIUM_CONSTANT_MEDIUM_H
#define NEARLIGHTER_MEDIUM_CONSTANT_MEDIUM_H

#include <nearlighter/scene/intersectable.h>
#include <nearlighter/texture/texture.h>

#include <memory>

class Primitive;

/**
 * Bounded Homogeneous Medium
 *
 * Represents a constant-density participating medium inside one closed,
 * convex Primitive boundary.
 * - Boundary: defines the occupied volume and its bounds.
 * - Density: defines the exponential free-flight distribution.
 * - Phase function: provides isotropic scattering at sampled events.
 */
class ConstantMedium final : public Intersectable {
public:
    /**
     * @name Medium Construction
     * Creates a medium from textured or constant isotropic attenuation.
     *
     * `boundary` must be non-null and closed convex. `density` is a positive
     * extinction coefficient measured in inverse distance in this entity's
     * coordinate space.
     * - Texture overload: retains the Texture by shared ownership.
     * - Color overload: creates a constant-color phase function.
     *
     * @throws std::invalid_argument if the boundary is null or density is not
     * positive.
     * @{
     */
    ConstantMedium(std::shared_ptr<const Primitive> boundary, float density,
                   std::shared_ptr<Texture> texture);
    ConstantMedium(std::shared_ptr<const Primitive> boundary, float density,
                   const Color& albedo);
    /** @} */

    /**
     * Stochastic Medium Interaction
     *
     * Samples the first free-flight event inside the accepted ray segment. On
     * success, `record` binds the isotropic phase Material; it is
     * unchanged on a miss.
     */
    bool hit(const Ray& ray, Interval ray_t, HitRecord& record,
             Sampler& sampler) const override;

    const AABB& getBoundingBox() const override;

private:
    std::shared_ptr<const Primitive> boundary_;
    float negative_inverse_density_;  // -1 / density for log(U) sampling.
    std::shared_ptr<const Material> phase_function_;
};

#endif  // NEARLIGHTER_MEDIUM_CONSTANT_MEDIUM_H
