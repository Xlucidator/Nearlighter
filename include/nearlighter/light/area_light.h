#ifndef NEARLIGHTER_LIGHT_AREA_LIGHT_H
#define NEARLIGHTER_LIGHT_AREA_LIGHT_H

#include <nearlighter/light/light.h>

class Primitive;

/**
 * Emissive Surface Light
 *
 * Borrows one world-space Primitive, which must outlive this light. Geometry,
 * placement and emission remain defined by that Primitive and its Material.
 */
class AreaLight final : public Light {
public:
    /** Requires an emissive Primitive with a surface-sampling distribution. */
    explicit AreaLight(const Primitive& primitive);

    std::optional<LightSample> sampleLi(
        const SurfaceInteraction& reference, float time,
        Sampler& sampler) const override;
    float PDFLi(const SurfaceInteraction& reference,
                const Vec3f& incoming) const override;

    const Primitive& primitive() const { return *primitive_; }

private:
    const Primitive* primitive_;
};

#endif  // NEARLIGHTER_LIGHT_AREA_LIGHT_H
