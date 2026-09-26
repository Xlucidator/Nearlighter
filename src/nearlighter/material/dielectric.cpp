#include <nearlighter/material/dielectric.h>

#include <nearlighter/math/math.h>
#include <nearlighter/sampling/sampler.h>
#include <nearlighter/scene/intersectable.h>
#include <nearlighter/scene/surface_interaction.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>

// ==================================================
// Dielectric Material
// ==================================================

Dielectric::Dielectric(float refractive_index)
    : refractive_index(refractive_index) {
    if (!std::isfinite(refractive_index) || refractive_index <= 0.0f) {
        throw std::invalid_argument(
            "Dielectric refractive index must be positive");
    }
}

std::optional<BSDF> Dielectric::computeBSDF(
    const SurfaceInteraction& interaction, TransportMode) const {
    BSDF bsdf(interaction.frame());
    // The frame is face-forward; frontFace still identifies the actual media.
    const float eta_i = interaction.frontFace() ? 1.0f : refractive_index;
    const float eta_t = interaction.frontFace() ? refractive_index : 1.0f;
    bsdf.add(std::make_unique<SpecularDielectricBxDF>(eta_i, eta_t));
    return bsdf;
}

bool Dielectric::scatter(const Ray& ray_in, const HitRecord& record,
                         ScatterRecord& s_record, Sampler& sampler) const {
    /* ----- Legacy Interface Geometry ----- */
    s_record.attenuation = Color(1.0f, 1.0f, 1.0f);
    s_record.sampling_pdf = nullptr;
    s_record.should_skip = true;

    float etai_over_etat =
        record.front_face ? (1.0f / refractive_index) : refractive_index;

    Vec3f unit_ray_in = unit_vector(ray_in.direction());
    float cos_theta_i = std::fmin(dot(-unit_ray_in, record.normal), 1.0f);
    float sin_theta_i = std::sqrt(1.0f - cos_theta_i * cos_theta_i);

    /* ----- Reflection Event Selection ----- */
    // Snell's law admits no transmitted direction when sin(theta_t) exceeds one.
    bool total_internal_reflection = (etai_over_etat * sin_theta_i) > 1.0f;

    // Preserve the legacy random stream: draw even under total internal reflection.
    bool fresnel_reflect =
        reflectance(cos_theta_i, etai_over_etat, 1.0f) > sampler.next1D();

    /* ----- Legacy Continuation ----- */
    Vec3f direction;
    if (total_internal_reflection || fresnel_reflect) {
        direction = reflect(unit_ray_in, record.normal);
    } else {
        direction = refract(unit_ray_in, record.normal, etai_over_etat);
    }

    s_record.skip_ray = Ray(record.point, direction, ray_in.time());
    return true;
}

float Dielectric::reflectance(float cos_theta_i, float eta_i, float eta_t) {
    float r0 = (eta_i - eta_t) / (eta_i + eta_t);
    r0 *= r0;
    return r0 + (1 - r0) * std::pow((1 - cos_theta_i), 5);
}

// ==================================================
// Local Dielectric Reflection and Transmission
// ==================================================

Color SpecularDielectricBxDF::evaluate(const Vec3f&, const Vec3f&,
                                       TransportMode) const {
    return Color();
}

float SpecularDielectricBxDF::PDF(const Vec3f&, const Vec3f&,
                                  TransportMode) const {
    return 0.0f;
}

std::optional<LocalBxDFSample> SpecularDielectricBxDF::sample(
    const Vec3f& outgoing, const Vec2f& direction_sample,
    TransportMode mode) const {
    /* ----- Interface Domain ----- */
    if (outgoing.z() == 0.0f || eta_i_ <= 0.0f || eta_t_ <= 0.0f) {
        return std::nullopt;
    }

    /* ----- Fresnel Reflection Event ----- */
    const float reflectance = fresnelDielectric(
        std::fabs(outgoing.z()), eta_i_, eta_t_);
    if (direction_sample.x() < reflectance) {
        const Vec3f incoming(-outgoing.x(), -outgoing.y(), outgoing.z());
        // Fresnel is both physical weight and event probability; cosine cancels.
        const float value = reflectance / std::fabs(incoming.z());
        return LocalBxDFSample{
            Color(value, value, value), incoming, reflectance,
            BxDFFlags::Reflection | BxDFFlags::Specular, 1.0f};
    }

    /* ----- Transmitted Direction ----- */
    const float eta_ratio = eta_i_ / eta_t_;
    const Vec3f normal = outgoing.z() > 0.0f
                             ? Vec3f(0.0f, 0.0f, 1.0f)
                             : Vec3f(0.0f, 0.0f, -1.0f);
    // refract() expects travel toward the surface, so negate outgoing.
    const Vec3f incoming = refract(-outgoing, normal, eta_ratio);
    if (incoming.near_zero()) {
        // Guard rounding disagreement with Fresnel near the critical angle.
        const Vec3f reflected(-outgoing.x(), -outgoing.y(), outgoing.z());
        const float value = 1.0f / std::fabs(reflected.z());
        return LocalBxDFSample{
            Color(value, value, value), reflected, 1.0f,
            BxDFFlags::Reflection | BxDFFlags::Specular, 1.0f};
    }

    /* ----- Transmission Transport Weight ----- */
    const float transmittance = 1.0f - reflectance;
    const float relative_eta = eta_t_ / eta_i_;
    float value = transmittance;
    if (mode == TransportMode::Radiance) {
        // Refraction changes the projected solid-angle measure. Radiance
        // transport needs (eta_i / eta_t)^2; importance transport omits it.
        value /= relative_eta * relative_eta;
    }
    value /= std::fabs(incoming.z());
    return LocalBxDFSample{
        Color(value, value, value), incoming, transmittance,
        BxDFFlags::Transmission | BxDFFlags::Specular, relative_eta};
}

float SpecularDielectricBxDF::fresnelDielectric(
    float cos_theta_i, float eta_i, float eta_t) {
    /* ----- Snell Transmission Domain ----- */
    if (eta_i <= 0.0f || eta_t <= 0.0f) {
        throw std::invalid_argument("Dielectric Fresnel indices must be positive");
    }
    cos_theta_i = std::clamp(std::fabs(cos_theta_i), 0.0f, 1.0f);
    const float sin_theta_i =
        std::sqrt(std::max(0.0f, 1.0f - cos_theta_i * cos_theta_i));
    const float sin_theta_t = eta_i * sin_theta_i / eta_t;
    if (sin_theta_t >= 1.0f) return 1.0f;

    /* ----- Unpolarized Reflectance ----- */
    const float cos_theta_t =
        std::sqrt(std::max(0.0f, 1.0f - sin_theta_t * sin_theta_t));
    const float parallel =
        (eta_t * cos_theta_i - eta_i * cos_theta_t) /
        (eta_t * cos_theta_i + eta_i * cos_theta_t);
    const float perpendicular =
        (eta_i * cos_theta_i - eta_t * cos_theta_t) /
        (eta_i * cos_theta_i + eta_t * cos_theta_t);
    // Average squared field-amplitude reflectances of the two polarizations.
    return 0.5f * (parallel * parallel + perpendicular * perpendicular);
}
