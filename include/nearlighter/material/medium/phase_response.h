#ifndef NEARLIGHTER_MATERIAL_MEDIUM_PHASE_RESPONSE_H
#define NEARLIGHTER_MATERIAL_MEDIUM_PHASE_RESPONSE_H

#include <nearlighter/material/medium/phase_function.h>
#include <nearlighter/material/scattering.h>

/**
 * Per-Event Volume Response after Extinction Free-Flight Sampling
 *
 * - evaluate() includes scattering albedo: alpha * phase.
 * - PDF() describes the conditional direction proposal, without albedo.
 * - Directions are world-space; no projected surface cosine applies.
 * - The immutable phase is borrowed and must outlive this response.
 */
class PhaseResponse final : public SampleableFunction<Vec3f, Vec3f, Color, std::optional<ScatteringSample>> {
public:
    /** Albedo Must Be Finite and in [0, 1] */
    PhaseResponse(Color albedo, const PhaseFunction& phase);

    Color evaluate(const Vec3f& outgoing, const Vec3f& incoming) const override;
    float PDF(const Vec3f& outgoing, const Vec3f& incoming) const override;
    std::optional<ScatteringSample> sample(
        const Vec3f& outgoing, Sampler& sampler) const override;

private:
    Color albedo_;
    const PhaseFunction* phase_;
};

#endif  // NEARLIGHTER_MATERIAL_MEDIUM_PHASE_RESPONSE_H
