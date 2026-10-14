#include <nearlighter/material/surface/lambertian.h>
#include <nearlighter/sampling/pdf.h>

#include <nearlighter/math/constants.h>
#include <nearlighter/render/hit/surface_record.h>
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

ScatteringResponse Lambertian::scattering(
    const HitRecord& hit, TransportMode) const {
    const auto& record = hit.surface();
    BSDF bsdf(ShadingFrame(record.normal));
    // Resolve the texture now; the returned BSDF retains only this hit's color.
    bsdf.add(std::make_unique<LambertianBxDF>(
        texture->value(record.u, record.v, record.point)));
    return bsdf;
}

// ==================================================
// Local Diffuse Reflection
// ==================================================

Color LambertianBxDF::evaluate(const Vec3f& outgoing, const Vec3f& incoming) const {
    // Reflection stays on outgoing's side; neither side is privileged.
    if (!(outgoing.z() * incoming.z() > 0.0f)) return Color();
    return albedo_ / pi;
}

float LambertianBxDF::PDF(const Vec3f& outgoing, const Vec3f& incoming) const {
    if (!(outgoing.z() * incoming.z() > 0.0f)) return 0.0f;
    // BxDF directions are already unit vectors in the local frame.
    return std::fabs(incoming.z()) / pi;
}

/**
 * @par Implementation
 * This proposal cancels the projected cosine in f * abs(cosine) / pdf.
 */
std::optional<ScatteringSample> LambertianBxDF::sample(
    const Vec3f& outgoing, Sampler& sampler) const {
    if (outgoing.z() == 0.0f) return std::nullopt;

    const CosineHemispherePDF proposal(Vec3f(0, 0, 1));
    Vec3f incoming = proposal.generate(sampler);
    if (outgoing.z() < 0.0f) incoming[2] = -incoming.z();
    return ScatteringSample{
        evaluate(outgoing, incoming), incoming,
        PDF(outgoing, incoming), flags(), 1.0f};
}
