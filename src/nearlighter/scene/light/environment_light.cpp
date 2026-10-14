#include <nearlighter/scene/light/environment_light.h>

#include <nearlighter/math/constants.h>
#include <nearlighter/sampling/pdf.h>

std::optional<LightSample> ConstantEnvironmentLight::sampleLi(
    const HitRecord&, float, Sampler& sampler) const {
    const SpherePDF proposal;
    const Vec3f incoming = proposal.generate(sampler);
    return LightSample{evaluateLi(incoming), incoming, proposal.value(incoming), infinity};
}

float ConstantEnvironmentLight::PDFLi(const HitRecord&,
                                       const Vec3f& incoming) const {
    return SpherePDF().value(incoming);
}
