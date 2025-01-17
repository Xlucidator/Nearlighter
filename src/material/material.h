#ifndef MATERIAL_H
#define MATERIAL_H

#include "shape/shape.h"

class Material {
public:
    virtual ~Material() = default;

    virtual bool scatter(const Ray& ray_in, const HitRecord& record, Color& attenuation, Ray& scattered) const 
    { return false; }

};

/* Derived Material Implement */
#include "lambertian.h"
#include "metal.h"

#endif // MATERIAL_H
