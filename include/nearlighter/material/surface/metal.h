#ifndef METAL_H
#define METAL_H

#include <nearlighter/material/material.h>

/** RGB Metal with Isotropic GGX Roughness and Schlick Reflectance */
class Metal : public Material {
public:
    /**
     * Metal Reflectance and Roughness
     *
     * @param albedo Normal-incidence RGB reflectance in [0, 1].
     * @param roughness Value in [0, 1]; alpha = roughness squared.
     * Alpha below 0.001 uses the ideal-reflection limit for numerical stability.
     */
    Metal(const Color& albedo, float roughness = 0.0f);

    MaterialKind kind() const override { return MaterialKind::Surface; }
    ScatteringResponse scattering(
        const HitRecord& record,
        TransportMode mode = TransportMode::Radiance) const override;

private:
    Color albedo;
    float roughness;
};

/**
 * Local Ideal Reflection with Constant RGB Reflectance
 *
 * No conductor optical constants or angle-dependent Fresnel are modeled.
 */
class SpecularReflectionBxDF : public BxDF {
public:
    explicit SpecularReflectionBxDF(Color reflectance)
        : reflectance_(reflectance) {}

    Color evaluate(const Vec3f& outgoing, const Vec3f& incoming) const override;
    float PDF(const Vec3f& outgoing, const Vec3f& incoming) const override;
    std::optional<ScatteringSample> sample(
        const Vec3f& outgoing, Sampler& sampler) const override;
    BxDFFlags flags() const override {
        return BxDFFlags::Reflection | BxDFFlags::Specular;
    }

private:
    Color reflectance_;
};

/** Isotropic GGX Reflection with Smith Masking and Visible-Normal Sampling */
class GGXReflectionBxDF final : public BxDF {
public:
    /** Positive GGX Width; Metal Selects the Ideal Limit for Very Small Widths */
    GGXReflectionBxDF(Color reflectance, float alpha);

    Color evaluate(const Vec3f& outgoing, const Vec3f& incoming) const override;
    float PDF(const Vec3f& outgoing, const Vec3f& incoming) const override;
    std::optional<ScatteringSample> sample(
        const Vec3f& outgoing, Sampler& sampler) const override;
    BxDFFlags flags() const override {
        return BxDFFlags::Reflection | BxDFFlags::Glossy;
    }

private:
    float normalDensity(const Vec3f& normal) const;
    float maskingLambda(const Vec3f& direction) const;
    Color reflectance_;
    float alpha_;
};

#endif  // METAL_H
