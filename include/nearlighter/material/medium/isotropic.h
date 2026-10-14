#ifndef NEARLIGHTER_MATERIAL_MEDIUM_ISOTROPIC_H
#define NEARLIGHTER_MATERIAL_MEDIUM_ISOTROPIC_H

#include <nearlighter/material/medium/phase_function.h>

/** Uniform Sphere Phase Function; Value and PDF Are 1 / (4 * pi) */
class Isotropic final : public PhaseFunction {
public:
    float evaluate(const Vec3f& outgoing, const Vec3f& incoming) const override;
    float PDF(const Vec3f& outgoing, const Vec3f& incoming) const override;
    std::optional<PhaseSample> sample(
        const Vec3f& outgoing, Sampler& sampler) const override;
};

#endif  // NEARLIGHTER_MATERIAL_MEDIUM_ISOTROPIC_H
