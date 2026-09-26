#ifndef NEARLIGHTER_LIGHT_LIGHT_H
#define NEARLIGHTER_LIGHT_LIGHT_H

#include <nearlighter/base/color.h>

#include <optional>

class Sampler;
class SurfaceInteraction;

/**
 * Incident Light Sample
 *
 * Sample contract:
 * - incoming is a world-space unit direction from the reference toward the light.
 * - radiance travels toward the reference, opposite to incoming.
 * - radiance excludes occlusion by other scene objects.
 * - pdf is a solid-angle density conditioned on selecting this light.
 * - distance is measured from reference.spawnRay(incoming, time).origin().
 * - Infinite distance denotes environment illumination.
 */
struct LightSample {
    Color radiance;
    Vec3f incoming;
    float pdf = 0.0f;
    float distance = 0.0f;
};

/** Incident Radiance Sampling */
class Light {
public:
    virtual ~Light() = default;

    /**
     * Incident Illumination at a Reference Point
     *
     * The caller checks visibility and combines the PDF with light selection.
     * @param reference Receiving surface point whose illumination is sampled.
     * @param time Scene time for the sampled geometry query.
     * @param sampler Random source for direction selection.
     * @return A sample toward this light, or nullopt when no sample is available.
     */
    virtual std::optional<LightSample> sampleLi(
        const SurfaceInteraction& reference, float time,
        Sampler& sampler) const = 0;

    /** Conditional solid-angle density for a world-space unit direction. */
    virtual float PDFLi(const SurfaceInteraction& reference,
                        const Vec3f& incoming) const = 0;
};

#endif  // NEARLIGHTER_LIGHT_LIGHT_H
