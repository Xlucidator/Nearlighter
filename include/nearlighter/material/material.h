#ifndef MATERIAL_H
#define MATERIAL_H

#include <nearlighter/geometry/shape.h>
#include <nearlighter/texture/texture.h>

class PDF;

struct ScatterRecord {
    Color attenuation;
    shared_ptr<PDF> pdf; // pdf pointer
    bool should_skip;    // skip using pdf to generate scattered ray
    Ray skip_ray;        // after skipping pdf, use targeted scattered ray
};

class Material {
public:
    virtual ~Material() = default;

    virtual Color emitted(
        const Ray& ray_in,        // Incoming ray that reaches the interaction.
        const HitRecord& record,  // Geometry interaction information.
        float u,                  // Horizontal texture coordinate.
        float v,                  // Vertical texture coordinate.
        const Point3f& p          // Interaction point used for texture evaluation.
    ) const
    {
        return Color(0, 0, 0);
    }

    virtual bool scatter(
        const Ray& ray_in,        // Incoming ray that reaches the interaction.
        const HitRecord& record,  // Geometry interaction information.
        ScatterRecord& s_record   // Output describing the scattering event.
    ) const
    {
        return false;
    }

    virtual float getScatterPDF(
        const Ray& ray_in,        // Incoming ray that reaches the interaction.
        const HitRecord& record,  // Geometry interaction information.
        Ray& ray_scattered        // Scattered ray whose PDF is evaluated.
    ) const
    {
        return 0.0f;
    }

};

#endif // MATERIAL_H
