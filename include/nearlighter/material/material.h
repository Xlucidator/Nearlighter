#ifndef MATERIAL_H
#define MATERIAL_H

#include <nearlighter/geometry/shape.h>
#include <nearlighter/texture/texture.h>

#include <memory>

class PDF;
class Sampler;

/** Material response used to continue one path at a surface interaction. */
struct ScatterRecord {
    Color attenuation;
    std::shared_ptr<PDF> sampling_pdf;
    bool should_skip;
    Ray skip_ray;
};

class Material {
public:
    virtual ~Material() = default;

    /** Returns radiance emitted from the interaction toward the incoming ray. */
    virtual Color emitted(
        [[maybe_unused]] const Ray& ray_in,        // Incoming ray that reaches the interaction.
        [[maybe_unused]] const HitRecord& record,  // Geometry interaction information.
        [[maybe_unused]] float u,                  // Horizontal texture coordinate.
        [[maybe_unused]] float v,                  // Vertical texture coordinate.
        [[maybe_unused]] const Point3f& p          // Interaction point used for texture evaluation.
    ) const { return Color(0, 0, 0); }

    /** Samples or describes the material response at an interaction. */
    virtual bool scatter(
        [[maybe_unused]] const Ray& ray_in,        // Incoming ray that reaches the interaction.
        [[maybe_unused]] const HitRecord& record,  // Geometry interaction information.
        [[maybe_unused]] ScatterRecord& s_record,  // Output describing the scattering event.
        [[maybe_unused]] Sampler& sampler
    ) const { return false; }

    /** Evaluates the material sampling density for a generated ray. */
    virtual float getScatterPDFValue(
        [[maybe_unused]] const Ray& ray_in,        // Incoming ray that reaches the interaction.
        [[maybe_unused]] const HitRecord& record,  // Geometry interaction information.
        [[maybe_unused]] const Ray& ray_scattered  // Scattered ray whose PDF is evaluated.
    ) const { return 0.0f; }

};

#endif // MATERIAL_H
