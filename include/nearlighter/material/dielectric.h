#ifndef DIELECTRIC_H
#define DIELECTRIC_H

#include <nearlighter/material/material.h>

/** Smooth Dielectric Material with Air as the Surrounding Medium */
class Dielectric : public Material {
public:
    Dielectric(float refractive_index);

    std::optional<BSDF> computeBSDF(
        const SurfaceInteraction& interaction,
        TransportMode mode) const override;
    bool supportsPathIntegrator() const override { return true; }

    // Legacy integrator hook; retains the original Schlick approximation.
    bool scatter(const Ray& ray_in, const HitRecord& record,
                 ScatterRecord& s_record, Sampler& sampler) const override;

private:
    float refractive_index;

    /** Legacy Schlick Reflectance; Separate from the BxDF's Exact Fresnel */
    static float reflectance(float cos_theta_i, float eta_i, float eta_t);
};

/**
 * Local Coupled Dielectric Reflection and Transmission
 *
 * One component samples both delta events, linked by unpolarized Fresnel.
 * Unlike a constant-RGB mirror, reflection varies with angle and both indices.
 */
class SpecularDielectricBxDF : public BxDF {
public:
    /**
     * Evaluated Interface Indices
     *
     * @param eta_i Positive index on outgoing's side, before the path step.
     * @param eta_t Positive index across the interface, after transmission.
     */
    SpecularDielectricBxDF(float eta_i, float eta_t)
        : eta_i_(eta_i), eta_t_(eta_t) {}

    Color evaluate(const Vec3f& outgoing, const Vec3f& incoming,
                   TransportMode mode) const override;
    float PDF(const Vec3f& outgoing, const Vec3f& incoming,
              TransportMode mode) const override;
    std::optional<LocalBxDFSample> sample(
        const Vec3f& outgoing, const Vec2f& direction_sample,
        TransportMode mode) const override;
    BxDFFlags flags() const override {
        return BxDFFlags::Reflection | BxDFFlags::Transmission |
               BxDFFlags::Specular;
    }

private:
    /** Exact Unpolarized Reflectance; One for Total Internal Reflection */
    static float fresnelDielectric(float cos_theta_i, float eta_i, float eta_t);

    float eta_i_ = 1.0f;
    float eta_t_ = 1.0f;
};

#endif // DIELECTRIC_H
