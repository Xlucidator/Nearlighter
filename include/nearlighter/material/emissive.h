#ifndef NEARLIGHTER_MATERIAL_EMISSIVE_H
#define NEARLIGHTER_MATERIAL_EMISSIVE_H

#include <nearlighter/material/material.h>

#include <memory>

class Texture;

/**
 * One-Sided Emissive Material
 *
 * Emitted radiance may vary over the surface but is direction-independent in
 * the outward hemisphere. Provides no surface scattering.
 */
class Emissive : public Material {
public:
    Emissive(std::shared_ptr<Texture> tex);
    Emissive(const Color& emit);

    std::optional<BSDF> computeBSDF(
        const SurfaceInteraction& interaction,
        TransportMode mode) const override;
    bool supportsPathIntegrator() const override { return true; }
    bool isEmissive() const override { return true; }
    Color evaluateEmission(const SurfaceInteraction& interaction,
                           const Vec3f& outgoing) const override;

    // Legacy integrator hook.
    Color emitted(const Ray& ray_in, const HitRecord& record,
                  float u, float v, const Point3f& p) const override;

private:
    std::shared_ptr<Texture> texture;
};

#endif // NEARLIGHTER_MATERIAL_EMISSIVE_H
