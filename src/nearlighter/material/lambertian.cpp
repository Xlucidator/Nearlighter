#include <nearlighter/material/lambertian.h>

#include <nearlighter/math/constants.h>
#include <nearlighter/sampling/pdf.h>
#include <nearlighter/scene/intersectable.h>
#include <nearlighter/scene/surface_interaction.h>
#include <nearlighter/texture/solid_texture.h>

#include <algorithm>
#include <cmath>

// ==================================================
// Diffuse Material
// ==================================================

Lambertian::Lambertian(const Color& albedo)
    : texture(std::make_shared<SolidTexture>(albedo)) {}

Lambertian::Lambertian(std::shared_ptr<Texture> tex)
    : texture(tex) {}

std::optional<BSDF> Lambertian::computeBSDF(
    const SurfaceInteraction& interaction, TransportMode) const {
    BSDF bsdf(interaction.frame());
    // Resolve the texture now; the returned BSDF retains only this hit's color.
    bsdf.add(std::make_unique<LambertianBxDF>(
        texture->value(interaction.u(), interaction.v(), interaction.point())));
    return bsdf;
}

bool Lambertian::scatter(const Ray&, const HitRecord& record,
                         ScatterRecord& s_record, Sampler&) const {
    s_record.attenuation = texture->value(record.u, record.v, record.point);
    s_record.sampling_pdf =
        std::make_shared<CosineHemispherePDF>(record.normal);
    s_record.should_skip = false;
    return true;
}

float Lambertian::getScatterPDFValue(const Ray&, const HitRecord& record,
                                     const Ray& ray_scattered) const {
    const float cosine =
        dot(record.normal, unit_vector(ray_scattered.direction()));
    return cosine > 0.0f ? cosine / pi : 0.0f;
}

// ==================================================
// Local Diffuse Reflection
// ==================================================

Color LambertianBxDF::evaluate(const Vec3f& outgoing, const Vec3f& incoming,
                               TransportMode) const {
    // Reflection stays on outgoing's side; neither side is privileged.
    if (!(outgoing.z() * incoming.z() > 0.0f)) return Color();
    return albedo_ / pi;
}

float LambertianBxDF::PDF(const Vec3f& outgoing, const Vec3f& incoming,
                          TransportMode) const {
    if (!(outgoing.z() * incoming.z() > 0.0f)) return 0.0f;
    return std::fabs(incoming.z()) / pi;
}

/**
 * @par Implementation
 * A uniform disk radius sqrt(u) projects to a cosine-weighted hemisphere.
 * This proposal cancels the projected cosine in f * abs(cosine) / pdf.
 */
std::optional<LocalBxDFSample> LambertianBxDF::sample(
    const Vec3f& outgoing, const Vec2f& direction_sample,
    TransportMode mode) const {
    if (outgoing.z() == 0.0f) return std::nullopt;

    const float phi = 2.0f * pi * direction_sample.x();
    const float sin_theta = std::sqrt(direction_sample.y());
    Vec3f incoming(sin_theta * std::cos(phi),
                   sin_theta * std::sin(phi),
                   std::sqrt(std::max(0.0f, 1.0f - direction_sample.y())));
    if (outgoing.z() < 0.0f) incoming[2] = -incoming.z();
    return LocalBxDFSample{
        evaluate(outgoing, incoming, mode), incoming,
        PDF(outgoing, incoming, mode), flags(), 1.0f};
}
