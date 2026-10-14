#ifndef NEARLIGHTER_MATERIAL_MEDIUM_PHASE_FUNCTION_H
#define NEARLIGHTER_MATERIAL_MEDIUM_PHASE_FUNCTION_H

#include <nearlighter/sampling/sampleable_function.h>
#include <nearlighter/math/vec3.h>
#include <optional>

/** Direction Sample with Separate Physical Response and Solid-Angle Density */
struct PhaseSample {
    float value;
    Vec3f incoming;
    float pdf;
};

/**
 * Volume Angular Scattering
 *
 * Both directions point away from the event in the same coordinate space.
 * Non-unit directions are accepted; invalid directions yield zero or no sample.
 * No surface frame, cosine, or refraction factor enters this response.
 */
class PhaseFunction : public SampleableFunction<Vec3f, Vec3f, float, std::optional<PhaseSample>> {
public:
    /** Normalized Angular Response; Its Integral over Incoming Solid Angle Is One */
    float evaluate(const Vec3f& outgoing, const Vec3f& incoming) const override = 0;
    /** Solid-Angle Density of the Direction Proposal; Need Not Equal the Response */
    float PDF(const Vec3f& outgoing, const Vec3f& incoming) const override = 0;
    /** Unit Continuation Direction; Value and Proposal Density Remain Separate */
    std::optional<PhaseSample> sample(
        const Vec3f& outgoing, Sampler& sampler) const override = 0;
};

#endif  // NEARLIGHTER_MATERIAL_MEDIUM_PHASE_FUNCTION_H
