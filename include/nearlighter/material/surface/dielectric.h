#ifndef DIELECTRIC_H
#define DIELECTRIC_H

#include <nearlighter/material/material.h>

/** Smooth Dielectric Material with Air as the Surrounding Medium */
class Dielectric : public Material {
public:
    Dielectric(float refractive_index);

    MaterialKind kind() const override { return MaterialKind::Surface; }
    ScatteringResponse scattering(
        const HitRecord& record,
        TransportMode mode = TransportMode::Radiance) const override;

private:
    float refractive_index;

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
     * @param eta_i Finite positive index on outgoing's side, before the path step.
     * @param eta_t Finite positive index across the interface, after transmission.
     * @param mode Transport operator fixed for this evaluated component.
     * @throws std::invalid_argument for non-finite or non-positive indices.
     */
    SpecularDielectricBxDF(float eta_i, float eta_t,
                           TransportMode mode = TransportMode::Radiance);

    Color evaluate(const Vec3f& outgoing, const Vec3f& incoming) const override;
    float PDF(const Vec3f& outgoing, const Vec3f& incoming) const override;
    std::optional<ScatteringSample> sample(
        const Vec3f& outgoing, Sampler& sampler) const override;
    BxDFFlags flags() const override {
        return BxDFFlags::Reflection | BxDFFlags::Transmission |
               BxDFFlags::Specular;
    }

private:
    /** Exact Unpolarized Reflectance; One for Total Internal Reflection */
    static float reflectanceExact(float cos_theta_i, float eta_i, float eta_t);

    /** Schlick Reflectance; One for Total Internal Reflection */
    static float reflectanceSchlick(float cos_theta_i, float eta_i, float eta_t);

    float eta_i_ = 1.0f;
    float eta_t_ = 1.0f;
    const TransportMode mode_;
};

#endif // DIELECTRIC_H
