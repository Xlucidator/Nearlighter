#include <nearlighter/light/area_light.h>

#include <nearlighter/base/interval.h>
#include <nearlighter/material/material.h>
#include <nearlighter/math/constants.h>
#include <nearlighter/scene/primitive.h>
#include <nearlighter/scene/surface_interaction.h>

#include <cmath>
#include <stdexcept>

AreaLight::AreaLight(const Primitive& primitive) : primitive_(&primitive) {
    if (!primitive.material().isEmissive() || !primitive.hasPDF()) {
        throw std::invalid_argument("AreaLight requires a sampleable emissive Primitive");
    }
}

std::optional<LightSample> AreaLight::sampleLi(
    const SurfaceInteraction& reference, float time, Sampler& sampler) const {
    const Vec3f direction = primitive_->random(reference.point(), sampler);
    if (direction.near_zero()) return std::nullopt;
    const Vec3f incoming = unit_vector(direction);
    const float pdf = primitive_->getPDFValue(reference.point(), incoming);
    if (!(pdf > 0.0f) || !std::isfinite(pdf)) return std::nullopt;

    // A direction may reach multiple points of a closed surface. Recover the
    // first one using the same offset ray that the visibility query will use.
    const Ray probe = reference.spawnRay(incoming, time);
    HitRecord record;
    if (!primitive_->hit(probe, Interval(0.0f, infinity), record, sampler)) {
        return std::nullopt;
    }
    const SurfaceInteraction interaction(probe, record);
    const Color radiance = primitive_->material().evaluateEmission(interaction, -incoming);
    return LightSample{radiance, incoming, pdf,
                       (record.point - probe.origin()).length()};
}

float AreaLight::PDFLi(const SurfaceInteraction& reference,
                       const Vec3f& incoming) const {
    return primitive_->getPDFValue(reference.point(), incoming);
}
