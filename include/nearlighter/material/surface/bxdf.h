#ifndef NEARLIGHTER_MATERIAL_BXDF_H
#define NEARLIGHTER_MATERIAL_BXDF_H

#include <nearlighter/material/scattering.h>
#include <nearlighter/sampling/sampleable_function.h>

#include <optional>

/**
 * Local Surface-Scattering Component
 *
 * One component may describe reflection, transmission, or both.
 * It owns evaluated parameters and does not query textures or the scene.
 *
 * Direction convention (unit vectors in the shading frame, normal = +z):
 * - Both directions point away from the hit, possibly on opposite sides.
 * - On a camera path A -> P -> B, outgoing is P -> A, opposite the hit ray.
 * - incoming is P -> B; incident light physically travels along -incoming.
 */
class BxDF : public SampleableFunction<Vec3f, Vec3f, Color, std::optional<ScatteringSample>> {
public:
    /**
     * Continuous Scattering Value
     *
     * @param outgoing Local direction toward the previous camera-path vertex.
     * @param incoming Local candidate direction; may come from a light sample.
     * @return f without the projected cosine; zero for delta-only models.
     */
    Color evaluate(const Vec3f& outgoing, const Vec3f& incoming) const override = 0;

    /**
     * Continuous Sampling Density
     *
     * Uses the same local directions as evaluate().
     * Delta events contribute zero to this solid-angle query.
     */
    float PDF(const Vec3f& outgoing, const Vec3f& incoming) const override = 0;

    /**
     * Local Scattering Event
     *
     * pdf includes this component's internal choices; BSDF adds component selection.
     * @param outgoing Local direction toward the previous camera-path vertex.
     * @return Local continuation sample, or nullopt when no event is available.
     */
    std::optional<ScatteringSample> sample(
        const Vec3f& outgoing, Sampler& sampler) const override = 0;

    /** Union of All Event Categories Supported by This Component */
    virtual BxDFFlags flags() const = 0;
};

#endif  // NEARLIGHTER_MATERIAL_BXDF_H
