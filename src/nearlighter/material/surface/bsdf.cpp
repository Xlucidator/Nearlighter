#include <nearlighter/material/surface/bsdf.h>
#include <nearlighter/sampling/sampler.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace {

bool finiteDirection(const Vec3f& value) {
    return std::isfinite(value.x()) && std::isfinite(value.y()) &&
           std::isfinite(value.z()) && !value.near_zero();
}

}  // namespace

// ==================================================
// BSDF Ownership
// ==================================================

BSDF::BSDF(BSDF&& other) noexcept
    : frame_(std::move(other.frame_)),
      components_(std::move(other.components_)),
      component_count_(std::exchange(other.component_count_, 0)) {}

BSDF& BSDF::operator=(BSDF&& other) noexcept {
    if (this == &other) return *this;

    frame_ = std::move(other.frame_);
    components_ = std::move(other.components_);
    // Moving the pointers alone would leave the source counting empty slots.
    component_count_ = std::exchange(other.component_count_, 0);
    return *this;
}

void BSDF::add(std::unique_ptr<BxDF> component) {
    if (!component) {
        throw std::invalid_argument("BSDF component must not be null");
    }
    if (component_count_ >= kMaximumComponents) {
        throw std::length_error("BSDF component capacity exceeded");
    }
    components_[component_count_++] = std::move(component);
}

// ==================================================
// BSDF Evaluation and Sampling
// ==================================================

Color BSDF::evaluate(const Vec3f& outgoing, const Vec3f& incoming) const {
    if (!finiteDirection(outgoing) || !finiteDirection(incoming)) {
        return Color();
    }

    const Vec3f local_outgoing = frame_.toLocal(unit_vector(outgoing));
    const Vec3f local_incoming = frame_.toLocal(unit_vector(incoming));
    Color value;
    for (std::size_t index = 0; index < component_count_; ++index) {
        value += components_[index]->evaluate(
            local_outgoing, local_incoming);
    }
    return value;
}

float BSDF::PDF(const Vec3f& outgoing, const Vec3f& incoming) const {
    if (empty() || !finiteDirection(outgoing) || !finiteDirection(incoming)) {
        return 0.0f;
    }

    const Vec3f local_outgoing = frame_.toLocal(unit_vector(outgoing));
    const Vec3f local_incoming = frame_.toLocal(unit_vector(incoming));
    float pdf = 0.0f;
    for (std::size_t index = 0; index < component_count_; ++index) {
        pdf += components_[index]->PDF(local_outgoing, local_incoming);
    }
    return pdf / static_cast<float>(component_count_);
}

std::optional<ScatteringSample> BSDF::sample(
    const Vec3f& outgoing, Sampler& sampler) const {
    /* ----- Input Domain ----- */
    if (empty() || !finiteDirection(outgoing)) return std::nullopt;

    /* ----- Local Component Proposal ----- */
    const Vec3f local_outgoing = frame_.toLocal(unit_vector(outgoing));
    const float component_sample = sampler.next1D();
    const std::size_t selected = std::min(
        static_cast<std::size_t>(component_sample * component_count_),
        component_count_ - 1);
    auto result = components_[selected]->sample(
        local_outgoing, sampler);
    if (!result || result->pdf <= 0.0f || !finiteDirection(result->incoming)) {
        return std::nullopt;
    }

    /* ----- Aggregate Response and Probability ----- */
    const float selection_pdf = 1.0f / static_cast<float>(component_count_);
    if (hasAnyFlag(result->flags, BxDFFlags::Specular)) {
        // Delta coefficients cannot be reconstructed through continuous queries.
        // Retain this component's event; selection is corrected by the denominator.
        result->pdf *= selection_pdf;
    } else if (component_count_ > 1) {
        // Any continuous component may contribute along the proposed direction.
        // Sum physical responses, but average the competing proposal densities.
        Color aggregate_value;
        float aggregate_pdf = 0.0f;
        for (std::size_t index = 0; index < component_count_; ++index) {
            aggregate_value += components_[index]->evaluate(
                local_outgoing, result->incoming);
            aggregate_pdf += components_[index]->PDF(
                local_outgoing, result->incoming);
        }
        result->value = aggregate_value;
        result->pdf = aggregate_pdf * selection_pdf;
    }

    /* ----- World-Space Continuation ----- */
    // ShadingFrame is orthonormal: rotation needs no solid-angle Jacobian.
    return ScatteringSample{result->value, frame_.toWorld(result->incoming),
                            result->pdf, result->flags, result->eta};
}

BxDFFlags BSDF::flags() const {
    BxDFFlags result = BxDFFlags::None;
    for (std::size_t i = 0; i < component_count_; ++i) result = result | components_[i]->flags();
    return result;
}
