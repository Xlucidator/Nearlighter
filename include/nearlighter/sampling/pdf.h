#ifndef NEARLIGHTER_SAMPLING_PDF_H
#define NEARLIGHTER_SAMPLING_PDF_H

#include <nearlighter/geometry/shading_frame.h>
#include <nearlighter/math/vec3.h>

#include <memory>
#include <vector>

class Sampler;
class Primitive;

/**
 * Continuous Solid-Angle Direction Proposal
 *
 * - value() and generate() describe the same distribution and coordinates.
 * - Density is measured per unit solid angle.
 * - Direction magnitude has no probabilistic meaning.
 */
class PDF {
public:
    virtual ~PDF() = default;

    /**
     * Direction Density
     *
     * Non-finite and zero directions have zero density.
     */
    virtual float value(const Vec3f& direction) const = 0;

    /**
     * Direction Proposal
     *
     * Borrows and advances the caller-owned sample stream.
     * An empty distribution returns a zero direction.
     */
    virtual Vec3f generate(Sampler& sampler) const = 0;
};

/** Uniform Sphere Distribution with Density 1 / (4 * pi) */
class SpherePDF final : public PDF {
public:
    float value(const Vec3f& direction) const override;
    Vec3f generate(Sampler& sampler) const override;
};

/** Cosine-Weighted Hemisphere Distribution */
class CosineHemispherePDF final : public PDF {
public:
    /** Builds the Hemisphere around a Nonzero Normal */
    explicit CosineHemispherePDF(const Vec3f& normal) : frame_(normal) {}

    float value(const Vec3f& direction) const override;
    Vec3f generate(Sampler& sampler) const override;

private:
    ShadingFrame frame_;
};

/**
 * Surface-Target Direction Distribution
 *
 * - Samples an equal-weight mixture of placed surfaces from a fixed origin.
 * - Shape supplies geometry; Primitive applies the solid-angle Jacobian.
 * - Material does not contribute to this proposal.
 */
class SurfacePDF final : public PDF {
public:
    /**
     * Surface-Target Mixture
     *
     * - Borrows targets; the array must outlive this distribution.
     * - Each target must be non-null and provide a surface sampling pair.
     * - An empty array has zero density and generates a zero direction.
     */
    SurfacePDF(const std::vector<std::shared_ptr<const Primitive>>& targets,
               const Point3f& origin);

    float value(const Vec3f& direction) const override;
    Vec3f generate(Sampler& sampler) const override;

private:
    const std::vector<std::shared_ptr<const Primitive>>& targets_;
    Point3f origin_;
};

/**
 * Weighted Mixture of Two Borrowed Distributions
 *
 * - Both components must outlive the mixture.
 * - Sampling and density evaluation use the same component weights.
 */
class MixturePDF final : public PDF {
public:
    /**
     * Mixture Construction
     *
     * first_probability must be finite and in [0, 1].
     * Invalid probabilities throw std::invalid_argument.
     */
    MixturePDF(const PDF& first, const PDF& second, float first_probability = 0.5f);

    float value(const Vec3f& direction) const override;
    Vec3f generate(Sampler& sampler) const override;

private:
    const PDF& first_;
    const PDF& second_;
    float first_probability_;
};

#endif  // NEARLIGHTER_SAMPLING_PDF_H
