#ifndef MATERIAL_H
#define MATERIAL_H

#include <nearlighter/geometry/shape.h>
#include <nearlighter/texture/texture.h>

class PDF;
class Sampler;

struct ScatterRecord {
    Color attenuation;
    shared_ptr<PDF> pdf; // pdf pointer
    bool should_skip;    // skip using pdf to generate scattered ray
    Ray skip_ray;        // after skipping pdf, use targeted scattered ray
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

    /** Evaluates the material scattering PDF for a generated ray. */
    virtual float getScatterPDF(
        [[maybe_unused]] const Ray& ray_in,        // Incoming ray that reaches the interaction.
        [[maybe_unused]] const HitRecord& record,  // Geometry interaction information.
        [[maybe_unused]] const Ray& ray_scattered  // Scattered ray whose PDF is evaluated.
    ) const { return 0.0f; }

};

#endif // MATERIAL_H
