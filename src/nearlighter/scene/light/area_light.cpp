#include <nearlighter/scene/light/area_light.h>

#include <nearlighter/base/interval.h>
#include <nearlighter/material/material.h>
#include <nearlighter/math/constants.h>
#include <nearlighter/scene/object/primitive.h>
#include <nearlighter/render/hit/hit_record.h>

#include <cmath>
#include <stdexcept>

AreaLight::AreaLight(const Primitive& primitive) : primitive_(&primitive) {
    if (!primitive.hasPDF() || !primitive.material().isEmissive()) {
        throw std::invalid_argument("AreaLight requires a sampleable emissive Primitive");
    }
}

std::optional<LightSample> AreaLight::sampleLi(
    const HitRecord& reference, float time, Sampler& sampler) const {
    const Vec3f direction = primitive_->random(reference.point(), sampler);
    if (direction.near_zero()) return std::nullopt;

    const Vec3f incoming = unit_vector(direction);
    const float pdf = primitive_->getPDFValue(reference.point(), incoming);
    if (!(pdf > 0.0f) || !std::isfinite(pdf)) return std::nullopt;

    // Closed surfaces may have multiple endpoints along one direction.
    // Recover the first using the same offset ray as the visibility query.
    const Ray probe = reference.spawnRay(incoming, time);
    HitRecord record;
    if (!primitive_->hit(probe, Interval(0.0f, infinity), record, sampler)) {
        return std::nullopt;
    }
    const Color radiance = record.material().emission(record, -incoming);
    return LightSample{radiance, incoming, pdf,
                       (record.point() - probe.origin()).length()};
}

float AreaLight::PDFLi(const HitRecord& reference,
                       const Vec3f& incoming) const {
    return primitive_->getPDFValue(reference.point(), incoming);
}
