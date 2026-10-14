#include <nearlighter/material/medium/isotropic.h>
#include <nearlighter/sampling/pdf.h>

#include <cmath>

namespace {
bool validDirection(const Vec3f& v) {
    return std::isfinite(v.x()) && std::isfinite(v.y()) &&
           std::isfinite(v.z()) && !v.near_zero();
}
}  // namespace

float Isotropic::evaluate(const Vec3f& outgoing, const Vec3f& incoming) const {
    return validDirection(outgoing) && validDirection(incoming)
        ? SpherePDF().value(incoming) : 0.0f;
}

float Isotropic::PDF(const Vec3f& outgoing, const Vec3f& incoming) const {
    return evaluate(outgoing, incoming);
}

std::optional<PhaseSample> Isotropic::sample(
    const Vec3f& outgoing, Sampler& sampler) const {
    if (!validDirection(outgoing)) return std::nullopt;
    const SpherePDF proposal;
    const Vec3f incoming = proposal.generate(sampler);
    // For this model only, the normalized phase and uniform proposal coincide.
    const float density = proposal.value(incoming);
    return PhaseSample{density, incoming, density};
}
