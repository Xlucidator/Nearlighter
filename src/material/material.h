#ifndef MATERIAL_H
#define MATERIAL_H

#include "shape/shape.h"
#include "texture/texture.h"

class Material {
public:
    virtual ~Material() = default;

    virtual Color emitted(const Ray& ray_in, const HitRecord& record, float u, float v, const Point3f& p) const { return Color(0, 0, 0); }
    virtual bool scatter(const Ray& ray_in, const HitRecord& record, Color& attenuation, Ray& scattered, float& pdf) const 
    { return false; }
    virtual float getScatterPDF(const Ray& ray_in, const HitRecord& record, Ray& ray_scattered) const { return 0; }

};

/* Derived Material Implement */
#include "lambertian.h"
#include "metal.h"
#include "dielectric.h"
#include "diffuselight.h"
#include "isotropic.h"

#endif // MATERIAL_H
