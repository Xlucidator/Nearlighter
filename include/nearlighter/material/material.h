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

    virtual Color emitted(const Ray& ray_in, const HitRecord& record, float u, float v, const Point3f& p) const;
    virtual bool scatter(const Ray& ray_in, const HitRecord& record, ScatterRecord& s_record) const;
    virtual float getScatterPDF(const Ray& ray_in, const HitRecord& record, Ray& ray_scattered) const;

};

#endif // MATERIAL_H
