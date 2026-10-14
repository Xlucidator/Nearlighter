#include <nearlighter/material/surface/metal.h>
#include <nearlighter/sampling/sampler.h>

#include <nearlighter/math/math.h>
#include <nearlighter/render/hit/surface_record.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>

// ==================================================
// Metal Material
// ==================================================

namespace {
Color reflectanceSchlick(const Color& normal_reflectance, float cosine) {
    const float weight = std::pow(1.0f - std::clamp(cosine, 0.0f, 1.0f), 5.0f);
    return normal_reflectance + (Color(1, 1, 1) - normal_reflectance) * weight;
}
}  // namespace

Metal::Metal(const Color& albedo, float roughness)
    : albedo(albedo), roughness(roughness) {
    if (!(roughness >= 0.0f && roughness <= 1.0f)) {
        throw std::invalid_argument("Metal roughness must lie in [0, 1]");
    }
    for (std::size_t axis = 0; axis < 3; ++axis) {
        if (!(albedo[axis] >= 0.0f && albedo[axis] <= 1.0f)) {
            throw std::invalid_argument("Metal reflectance must lie in [0, 1]");
        }
    }
}

ScatteringResponse Metal::scattering(
    const HitRecord& hit, TransportMode) const {
    const auto& record = hit.surface();
    BSDF bsdf(ShadingFrame(record.normal));
    const float alpha = roughness * roughness;
    if (alpha < 0.001f) {
        const float cosine = std::fabs(dot(record.outgoing, record.normal));
        bsdf.add(std::make_unique<SpecularReflectionBxDF>(reflectanceSchlick(albedo, cosine)));
    } else {
        bsdf.add(std::make_unique<GGXReflectionBxDF>(albedo, alpha));
    }
    return bsdf;
}

// ==================================================
// Local Ideal Reflection
// ==================================================

Color SpecularReflectionBxDF::evaluate(const Vec3f&, const Vec3f&) const {
    return Color();
}

float SpecularReflectionBxDF::PDF(const Vec3f&, const Vec3f&) const {
    return 0.0f;
}

std::optional<ScatteringSample> SpecularReflectionBxDF::sample(
    const Vec3f& outgoing, Sampler&) const {
    if (outgoing.z() == 0.0f) return std::nullopt;

    // Reflect -outgoing, the arriving travel direction, about the local normal.
    const Vec3f incoming(-outgoing.x(), -outgoing.y(), outgoing.z());
    // The delta coefficient cancels the integrator's projected cosine.
    return ScatteringSample{
        reflectance_ / std::fabs(incoming.z()), incoming, 1.0f, flags(), 1.0f};
}

// ==================================================
// Local Rough Reflection
// ==================================================

GGXReflectionBxDF::GGXReflectionBxDF(Color reflectance, float alpha)
    : reflectance_(reflectance), alpha_(alpha) {
    if (!std::isfinite(alpha) || alpha <= 0.0f) {
        throw std::invalid_argument("GGX width must be finite and positive");
    }
}

float GGXReflectionBxDF::normalDensity(const Vec3f& normal) const {
    // This equivalent GGX form avoids tan(theta) overflow near grazing angles.
    const float alpha_squared = alpha_ * alpha_;
    const float denominator = normal.x() * normal.x() + normal.y() * normal.y() +
                              alpha_squared * normal.z() * normal.z();
    return alpha_squared / (pi * denominator * denominator);
}

float GGXReflectionBxDF::maskingLambda(const Vec3f& direction) const {
    const float tangent_squared = (direction.x() * direction.x() +
                                   direction.y() * direction.y()) /
                                  (direction.z() * direction.z());
    return 0.5f * (std::sqrt(1.0f + alpha_ * alpha_ * tangent_squared) - 1.0f);
}

Color GGXReflectionBxDF::evaluate(const Vec3f& outgoing, const Vec3f& incoming) const {
    if (!(outgoing.z() * incoming.z() > 0.0f)) return Color();
    const Vec3f normal = unit_vector(outgoing + incoming);
    const float masking = 1.0f / (1.0f + maskingLambda(outgoing) + maskingLambda(incoming));
    const Color reflectance = reflectanceSchlick(reflectance_, std::fabs(dot(outgoing, normal)));
    return reflectance * (normalDensity(normal) * masking /
                          (4.0f * std::fabs(outgoing.z() * incoming.z())));
}

float GGXReflectionBxDF::PDF(const Vec3f& outgoing, const Vec3f& incoming) const {
    if (!(outgoing.z() * incoming.z() > 0.0f)) return 0.0f;
    const Vec3f normal = unit_vector(outgoing + incoming);
    // Visible-normal density times reflection's 1 / (4 * |wo dot m|) Jacobian.
    return normalDensity(normal) /
           (4.0f * std::fabs(outgoing.z()) * (1.0f + maskingLambda(outgoing)));
}

/**
 * @par Implementation
 * Stretch outgoing to a unit-roughness hemisphere, sample its visible disk, then unstretch.
 * Rejected below-surface reflections represent zero contribution; never resample them.
 */
std::optional<ScatteringSample> GGXReflectionBxDF::sample(
    const Vec3f& outgoing, Sampler& sampler) const {
    if (outgoing.z() == 0.0f) return std::nullopt;

    /* ----- Stretched Viewing Frame ----- */
    const float side = outgoing.z() > 0.0f ? 1.0f : -1.0f;
    const Vec3f view = unit_vector(side * Vec3f(alpha_ * outgoing.x(),
                                              alpha_ * outgoing.y(), outgoing.z()));
    const Vec3f tangent = view.z() < 0.99999f
        ? unit_vector(cross(Vec3f(0, 0, 1), view)) : Vec3f(1, 0, 0);
    const Vec3f bitangent = cross(view, tangent);

    /* ----- Visible Hemisphere Proposal ----- */
    const Vec2f direction_sample = sampler.next2D();
    const float radius = std::sqrt(direction_sample.x());
    const float phi = 2.0f * pi * direction_sample.y();
    const float x = radius * std::cos(phi);
    const float blend = 0.5f * (1.0f + view.z());
    const float y = (1.0f - blend) * std::sqrt(std::max(0.0f, 1.0f - x * x)) +
                    blend * radius * std::sin(phi);
    const float z = std::sqrt(std::max(0.0f, 1.0f - x * x - y * y));
    const Vec3f stretched = x * tangent + y * bitangent + z * view;
    const Vec3f normal = side * unit_vector(Vec3f(
        alpha_ * stretched.x(), alpha_ * stretched.y(), std::max(1e-6f, stretched.z())));

    /* ----- Reflected Direction and Matching Density ----- */
    const Vec3f incoming = reflect(-outgoing, normal);
    if (!(outgoing.z() * incoming.z() > 0.0f)) return std::nullopt;
    return ScatteringSample{evaluate(outgoing, incoming), incoming,
                          PDF(outgoing, incoming), flags(), 1.0f};
}
