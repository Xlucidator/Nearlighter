#include <nearlighter/material/medium/phase_response.h>

#include <stdexcept>

PhaseResponse::PhaseResponse(Color albedo, const PhaseFunction& phase)
    : albedo_(albedo), phase_(&phase) {
    for (std::size_t axis = 0; axis < 3; ++axis) {
        if (!(albedo[axis] >= 0.0f && albedo[axis] <= 1.0f)) {
            throw std::invalid_argument("Phase response albedo must lie in [0, 1]");
        }
    }
}

Color PhaseResponse::evaluate(const Vec3f& outgoing, const Vec3f& incoming) const {
    return albedo_ * phase_->evaluate(outgoing, incoming);
}

float PhaseResponse::PDF(const Vec3f& outgoing, const Vec3f& incoming) const {
    return phase_->PDF(outgoing, incoming);
}

std::optional<ScatteringSample> PhaseResponse::sample(
    const Vec3f& outgoing, Sampler& sampler) const {
    const auto sample = phase_->sample(outgoing, sampler);
    if (!sample) return std::nullopt;
    return ScatteringSample{albedo_ * sample->value, sample->incoming, sample->pdf};
}
