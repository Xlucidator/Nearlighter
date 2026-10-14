#ifndef NEARLIGHTER_SCENE_LIGHT_AREA_LIGHT_H
#define NEARLIGHTER_SCENE_LIGHT_AREA_LIGHT_H

#include <nearlighter/scene/light/light.h>

class Primitive;

/**
 * Emissive Surface Light
 *
 * Source and lifetime:
 * - Borrows one world-space Primitive, which must outlive this light.
 * - Geometry and placement remain defined by that Primitive.
 * - Emission remains defined by its Material.
 */
class AreaLight final : public Light {
public:
    /** Requires an emissive Primitive with a surface-sampling distribution. */
    explicit AreaLight(const Primitive& primitive);

    std::optional<LightSample> sampleLi(
        const HitRecord& reference, float time,
        Sampler& sampler) const override;
    float PDFLi(const HitRecord& reference,
                const Vec3f& incoming) const override;

    const Primitive& primitive() const { return *primitive_; }

private:
    const Primitive* primitive_;
};

#endif  // NEARLIGHTER_SCENE_LIGHT_AREA_LIGHT_H
