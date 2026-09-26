#ifndef METAL_H
#define METAL_H

#include <nearlighter/material/material.h>

/** Constant-RGB Mirror Material; Nonzero Fuzz Is Legacy-Only */
class Metal : public Material {
public:
    Metal(const Color& albedo, float fuzz = 0.0f);

    std::optional<BSDF> computeBSDF(
        const SurfaceInteraction& interaction,
        TransportMode mode) const override;
    bool supportsPathIntegrator() const override { return fuzz == 0.0f; }

    // Legacy integrator hook, including the heuristic fuzzy reflection.
    bool scatter(const Ray& ray_in, const HitRecord& record,
                 ScatterRecord& s_record, Sampler& sampler) const override;

private:
    Color albedo;
    float fuzz;
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

    Color evaluate(const Vec3f& outgoing, const Vec3f& incoming,
                   TransportMode mode) const override;
    float PDF(const Vec3f& outgoing, const Vec3f& incoming,
              TransportMode mode) const override;
    std::optional<LocalBxDFSample> sample(
        const Vec3f& outgoing, const Vec2f& direction_sample,
        TransportMode mode) const override;
    BxDFFlags flags() const override {
        return BxDFFlags::Reflection | BxDFFlags::Specular;
    }

private:
    Color reflectance_;
};

#endif  // METAL_H
