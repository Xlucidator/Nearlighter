#ifndef NEARLIGHTER_LIGHT_ENVIRONMENT_LIGHT_H
#define NEARLIGHTER_LIGHT_ENVIRONMENT_LIGHT_H

#include <nearlighter/light/light.h>

/** Illumination from Directions at Infinity */
class EnvironmentLight : public Light {
public:
    /** Radiance seen by a ray escaping toward the world-space unit direction. */
    virtual Color evaluateLi(const Vec3f& incoming) const = 0;
};

/** Constant Environment Radiance */
class ConstantEnvironmentLight final : public EnvironmentLight {
public:
    explicit ConstantEnvironmentLight(Color radiance) : radiance_(radiance) {}

    std::optional<LightSample> sampleLi(
        const SurfaceInteraction& reference, float time,
        Sampler& sampler) const override;
    float PDFLi(const SurfaceInteraction& reference,
                const Vec3f& incoming) const override;
    Color evaluateLi([[maybe_unused]] const Vec3f& incoming) const override {
        return radiance_;
    }

private:
    Color radiance_;
};

#endif  // NEARLIGHTER_LIGHT_ENVIRONMENT_LIGHT_H
