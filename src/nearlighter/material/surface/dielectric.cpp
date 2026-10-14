#include <nearlighter/material/surface/dielectric.h>
#include <nearlighter/sampling/sampler.h>

#include <nearlighter/math/math.h>
#include <nearlighter/render/hit/surface_record.h>

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

ScatteringResponse Dielectric::scattering(
    const HitRecord& hit, TransportMode mode) const {
    const auto& record = hit.surface();
    BSDF bsdf(ShadingFrame(record.normal));
    // The frame is face-forward; front_face still identifies the actual media.
    const float eta_i = record.front_face ? 1.0f : refractive_index;
    const float eta_t = record.front_face ? refractive_index : 1.0f;
    bsdf.add(std::make_unique<SpecularDielectricBxDF>(eta_i, eta_t, mode));
    return bsdf;
}

// ==================================================
// Local Dielectric Reflection and Transmission
// ==================================================

SpecularDielectricBxDF::SpecularDielectricBxDF(
    float eta_i, float eta_t, TransportMode mode)
    : eta_i_(eta_i), eta_t_(eta_t), mode_(mode) {
    if (!std::isfinite(eta_i_) || !std::isfinite(eta_t_) ||
        eta_i_ <= 0.0f || eta_t_ <= 0.0f) {
        throw std::invalid_argument("Dielectric interface indices must be finite and positive");
    }
}

Color SpecularDielectricBxDF::evaluate(const Vec3f&, const Vec3f&) const {
    return Color();
}

float SpecularDielectricBxDF::PDF(const Vec3f&, const Vec3f&) const {
    return 0.0f;
}

std::optional<ScatteringSample> SpecularDielectricBxDF::sample(
    const Vec3f& outgoing, Sampler& sampler) const {
    /* ----- Interface Domain ----- */
    if (outgoing.z() == 0.0f) {
        return std::nullopt;
    }

    /* ----- Fresnel Reflection Event ----- */
    const float reflectance = reflectanceExact(
        std::fabs(outgoing.z()), eta_i_, eta_t_);
    if (sampler.next1D() < reflectance) {
        const Vec3f incoming(-outgoing.x(), -outgoing.y(), outgoing.z());
        // Fresnel is both physical weight and event probability; cosine cancels.
        const float value = reflectance / std::fabs(incoming.z());
        return ScatteringSample{
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
        return ScatteringSample{
            Color(value, value, value), reflected, 1.0f,
            BxDFFlags::Reflection | BxDFFlags::Specular, 1.0f};
    }

    /* ----- Transmission Transport Weight ----- */
    const float transmittance = 1.0f - reflectance;
    const float relative_eta = eta_t_ / eta_i_;
    float value = transmittance;
    if (mode_ == TransportMode::Radiance) {
        // Refraction changes the projected solid-angle measure.
        // Radiance needs (eta_i / eta_t)^2; importance omits this factor.
        value /= relative_eta * relative_eta;
    }
    value /= std::fabs(incoming.z());
    return ScatteringSample{
        Color(value, value, value), incoming, transmittance,
        BxDFFlags::Transmission | BxDFFlags::Specular, relative_eta};
}

float SpecularDielectricBxDF::reflectanceExact(
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

float SpecularDielectricBxDF::reflectanceSchlick(
    float cos_theta_i, float eta_i, float eta_t) {
    cos_theta_i = std::clamp(std::fabs(cos_theta_i), 0.0f, 1.0f);
    const float ratio = eta_i / eta_t;
    if (ratio * ratio * (1.0f - cos_theta_i * cos_theta_i) >= 1.0f) return 1.0f;
    float r0 = (eta_i - eta_t) / (eta_i + eta_t);
    r0 *= r0;
    return r0 + (1.0f - r0) * std::pow(1.0f - cos_theta_i, 5.0f);
}
