#ifndef NEARLIGHTER_MATERIAL_MEDIUM_HOMOGENEOUS_H
#define NEARLIGHTER_MATERIAL_MEDIUM_HOMOGENEOUS_H

#include <nearlighter/material/material.h>
#include <memory>

class Texture;

/**
 * Constant-Extinction Medium
 *
 * Extinction is constant; scattering albedo may use a spatial texture.
 * Phase is shared and immutable; omission selects Isotropic.
 */
class Homogeneous final : public Material {
public:
    /**
     * @name Medium Parameters
     * Extinction is finite and positive, in inverse Primitive input-space distance.
     * Albedo is finite and in [0, 1]; textures must produce that same range.
     * @{
     */
    Homogeneous(float extinction, Color albedo,
                std::shared_ptr<const PhaseFunction> phase = {});
    Homogeneous(float extinction, std::shared_ptr<const Texture> albedo,
                std::shared_ptr<const PhaseFunction> phase = {});
    /** @} */

    float extinction() const { return extinction_; }
    MaterialKind kind() const override { return MaterialKind::Medium; }
    ScatteringResponse scattering(
        const HitRecord& record,
        TransportMode mode = TransportMode::Radiance) const override;

private:
    float extinction_;
    std::shared_ptr<const Texture> albedo_;
    std::shared_ptr<const PhaseFunction> phase_;
};

#endif  // NEARLIGHTER_MATERIAL_MEDIUM_HOMOGENEOUS_H
