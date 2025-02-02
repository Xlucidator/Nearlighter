#ifndef LAMBERTIAN_H
#define LAMBERTIAN_H

#include "material.h"

class Lambertian : public Material {
public:
    Lambertian(const Color& albedo) : albedo(albedo) {}

    bool scatter(const Ray& ray_in, const HitRecord& record, Color& attenuation, Ray& scattered) const override {
        Vec3f scatter_direction = record.normal + Vec3f::random_unit_vector();

        // fix degenerate scatter direction
        if (scatter_direction.near_zero())
            scatter_direction = record.normal;

        scattered = Ray(record.point, scatter_direction, ray_in.time());
        attenuation = albedo;
        return true;
    }

private:
    Color albedo;
};


#endif // LAMBERTIAN_H
