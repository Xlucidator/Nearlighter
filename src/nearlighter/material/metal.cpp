#include <nearlighter/material/metal.h>

#include <nearlighter/math/math.h>
#include <nearlighter/sampling/sampler.h>
#include <nearlighter/scene/intersectable.h>
#include <nearlighter/scene/surface_interaction.h>

#include <cmath>
#include <stdexcept>

// ==================================================
// Mirror Material
// ==================================================

Metal::Metal(const Color& albedo, float fuzz)
    : albedo(albedo), fuzz(fuzz < 1.0f ? fuzz : 1.0f) {}

std::optional<BSDF> Metal::computeBSDF(
    const SurfaceInteraction& interaction, TransportMode) const {
    if (fuzz != 0.0f) {
        throw std::runtime_error("Fuzzy Metal has no BSDF implementation");
    }
    BSDF bsdf(interaction.frame());
    bsdf.add(std::make_unique<SpecularReflectionBxDF>(albedo));
    return bsdf;
}

bool Metal::scatter(const Ray& ray_in, const HitRecord& record,
                    ScatterRecord& s_record, Sampler& sampler) const {
    // Keep the legacy perturbation and its random draw, even when fuzz is zero.
    Vec3f reflect_direction = reflect(ray_in.direction(), record.normal);
    reflect_direction = unit_vector(reflect_direction) +
                        (fuzz * sampler.nextUnitVector());
    Ray scattered(record.point, reflect_direction, ray_in.time());

    s_record.attenuation = albedo;
    s_record.sampling_pdf = nullptr;
    s_record.should_skip = true;
    s_record.skip_ray = scattered;
    return dot(scattered.direction(), record.normal) > 0;
}

// ==================================================
// Local Ideal Reflection
// ==================================================

Color SpecularReflectionBxDF::evaluate(const Vec3f&, const Vec3f&,
                                       TransportMode) const {
    return Color();
}

float SpecularReflectionBxDF::PDF(const Vec3f&, const Vec3f&,
                                  TransportMode) const {
    return 0.0f;
}

std::optional<LocalBxDFSample> SpecularReflectionBxDF::sample(
    const Vec3f& outgoing, const Vec2f&, TransportMode) const {
    if (outgoing.z() == 0.0f) return std::nullopt;

    // Reflect -outgoing, the arriving travel direction, about the local normal.
    const Vec3f incoming(-outgoing.x(), -outgoing.y(), outgoing.z());
    // The delta coefficient cancels the integrator's projected cosine.
    return LocalBxDFSample{
        reflectance_ / std::fabs(incoming.z()), incoming, 1.0f, flags(), 1.0f};
}
