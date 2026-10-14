#ifndef NEARLIGHTER_MATERIAL_EMISSIVE_H
#define NEARLIGHTER_MATERIAL_EMISSIVE_H

#include <nearlighter/material/material.h>

#include <memory>

class Texture;

/**
 * One-Sided Emissive Material
 *
 * Emitted radiance may vary across the surface.
 * It is direction-independent within the outward hemisphere.
 * The material provides no surface scattering.
 */
class Emissive : public Material {
public:
    Emissive(std::shared_ptr<Texture> tex);
    Emissive(const Color& emit);

    MaterialKind kind() const override { return MaterialKind::Surface; }
    ScatteringResponse scattering(
        const HitRecord& record,
        TransportMode mode = TransportMode::Radiance) const override;
    bool isEmissive() const override { return true; }
    Color emission(const HitRecord& record, const Vec3f& outgoing) const override;

private:
    std::shared_ptr<Texture> texture;
};

#endif // NEARLIGHTER_MATERIAL_EMISSIVE_H
