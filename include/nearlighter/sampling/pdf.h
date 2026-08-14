#ifndef PDF_H
#define PDF_H 

#include <nearlighter/geometry/onb.h>
#include <nearlighter/math/constants.h>
#include <nearlighter/sampling/sampler.h>
#include <nearlighter/scene/primitive.h>

#include <cmath>
#include <memory>
#include <vector>

/**
 * Solid-Angle Direction Distribution
 *
 * Represents one distribution over directions on the unit sphere.
 * - `value()`: evaluates density with respect to solid angle.
 * - `generate()`: samples a direction from the same distribution.
 *
 * Direction length has no probabilistic meaning. Implementations must keep
 * both operations consistent to preserve unbiased Monte Carlo estimates.
 */
class PDF {
public:
    virtual ~PDF() = default;

    /**
     * Direction Density
     *
     * @param direction Non-zero direction in the distribution's coordinate
     * system.
     * @return Density with respect to solid angle, in inverse steradians.
     */
    virtual float value(const Vec3f& direction) const = 0;

    /**
     * Direction Generation
     *
     * @param sampler Random stream consumed by the distribution.
     * @return A non-zero direction distributed according to `value()`.
     */
    virtual Vec3f generate(Sampler& sampler) const = 0;
};

/**
 * Uniform Sphere Distribution
 *
 * Assigns constant density `1 / (4*pi)` to every direction.
 */
class SpherePDF : public PDF {
public:
    SpherePDF() {}

    float value([[maybe_unused]] const Vec3f& direction) const override {
        return 1 / (4 * pi);    
    }
    Vec3f generate(Sampler& sampler) const override {
        return sampler.nextUnitVector();
    }
};

/**
 * Cosine-Weighted Hemisphere Distribution
 *
 * Uses the constructor direction as the positive local z axis. The direction
 * must be non-zero; generated and evaluated directions use the parent space.
 */
class CosineHemispherePDF : public PDF {
public:
    CosineHemispherePDF(const Vec3f& w) : uvw_(w) {}

    float value(const Vec3f& direction) const override {
        float cosine_theta = dot(unit_vector(direction), uvw_.w());
        return std::fmax(0, cosine_theta / pi);
    }
    Vec3f generate(Sampler& sampler) const override {
        return uvw_.toParent(sampler.nextCosineHemisphere());
    }
private:
    ONB uvw_;
};

/**
 * Surface-Target Direction Distribution
 *
 * Forms an equal-weight mixture over world-space Primitive targets as viewed
 * from one fixed origin. Shape supplies surface sampling; Primitive supplies
 * placement and the affine solid-angle Jacobian. Material does not contribute.
 */
class SurfacePDF : public PDF {
public:
    /**
     * Surface-Target Mixture
     *
     * Retains `targets` by reference, so the array must outlive this object.
     * A valid distribution requires at least one non-null target with a valid
     * Shape sampling pair. An empty array remains safe but has zero density.
     */
    SurfacePDF(
        const std::vector<std::shared_ptr<const Primitive>>& targets,
        const Point3f& origin)
        : targets_(targets), origin_(origin) {}

    float value(const Vec3f& direction) const override {
        if (targets_.empty()) return 0.0f;
        const float weight = 1.0f / static_cast<float>(targets_.size());
        float density = 0.0f;
        for (const auto& target : targets_) {
            density += weight * target->getPDFValue(origin_, direction);
        }
        return density;
    }

    Vec3f generate(Sampler& sampler) const override {
        if (targets_.empty()) return Vec3f(1.0f, 0.0f, 0.0f);
        const int index = sampler.nextInt(
            0, static_cast<int>(targets_.size()) - 1);
        return targets_[static_cast<std::size_t>(index)]->random(origin_, sampler);
    }
private:
    const std::vector<std::shared_ptr<const Primitive>>& targets_;
    Point3f origin_;
};

/**
 * Equal-Weight Direction Mixture
 *
 * Samples either component with probability one half and evaluates the same
 * weighted sum of their densities.
 */
class MixturePDF : public PDF {
public:
    /**
     * Mixture Construction
     *
     * Retains both non-null component distributions by shared ownership.
     */
    MixturePDF(std::shared_ptr<PDF> p0, std::shared_ptr<PDF> p1) {
        p[0] = p0;
        p[1] = p1;
    }

    float value(const Vec3f& direction) const override {
        return 0.5 * p[0]->value(direction) + 0.5 * p[1]->value(direction);
    }
    Vec3f generate(Sampler& sampler) const override {
        if (sampler.next1D() < 0.5f)
            return p[0]->generate(sampler);
        else 
            return p[1]->generate(sampler);
    }

private:
    std::shared_ptr<PDF> p[2];
};

#endif // PDF_H
