#ifndef MATERIAL_H
#define MATERIAL_H

#include "shape/shape.h"
#include "texture/texture.h"

class Material {
public:
    virtual ~Material() = default;

    virtual Color emitted(float u, float v, const Point3f& p) const { return Color(0, 0, 0); }
    virtual bool scatter(const Ray& ray_in, const HitRecord& record, Color& attenuation, Ray& scattered) const 
    { return false; }

};

/* Derived Material Implement */
#include "lambertian.h"
#include "metal.h"
#include "dielectric.h"
#include "diffuselight.h"
#include "isotropic.h"

#endif // MATERIAL_H
