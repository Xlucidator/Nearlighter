#include <nearlighter/light/environment_light.h>

#include <nearlighter/math/constants.h>
#include <nearlighter/sampling/sampler.h>

#include <algorithm>
#include <cmath>

std::optional<LightSample> ConstantEnvironmentLight::sampleLi(
    const SurfaceInteraction&, float, Sampler& sampler) const {
    const Vec2f sample = sampler.next2D();
    const float z = 1.0f - 2.0f * sample.x();
    const float radius = std::sqrt(std::max(0.0f, 1.0f - z * z));
    const float phi = 2.0f * pi * sample.y();
    const Vec3f incoming(radius * std::cos(phi), radius * std::sin(phi), z);
    return LightSample{evaluateLi(incoming), incoming, 1.0f / (4.0f * pi), infinity};
}

float ConstantEnvironmentLight::PDFLi(const SurfaceInteraction&,
                                       const Vec3f& incoming) const {
    return incoming.near_zero() ? 0.0f : 1.0f / (4.0f * pi);
}
