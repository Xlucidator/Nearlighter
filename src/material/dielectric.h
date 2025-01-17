#ifndef DIELECTRIC_H
#define DIELECTRIC_H

#include "material.h"

class Dielectric : public Material {
public:
    Dielectric(float refractive_index) : refractive_index(refractive_index) {}

    bool scatter(const Ray& ray_in, const HitRecord& record, Color& attenuation, Ray& scattered) const override {
        float etai_over_etao = record.front_face ? (1.0f / refractive_index) : refractive_index;
        Vec3f refract_direction = refract(unit_vector(ray_in.direction()), record.normal, etai_over_etao);

        scattered = Ray(record.point, refract_direction);
        attenuation = Color(1.0f, 1.0f, 1.0f);
        return true;
    }

private:
    float refractive_index; // refractive index in vacuum, or index over the other enclosing media
};


#endif // DIELECTRIC_H
