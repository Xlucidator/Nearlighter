#ifndef METAL_H
#define METAL_H

#include "material.h"

class Metal : public Material {
public:
    Metal(const Color& albedo, float fuzz = 0.0f) : albedo(albedo), fuzz(fuzz < 1.0f ? fuzz : 1.0f) {}

    bool scatter(
        const Ray& ray_in, const HitRecord& record, Color& attenuation, Ray& scattered,
        float& pdf
    ) const override {
        Vec3f reflect_direction = reflect(ray_in.direction(), record.normal);
        reflect_direction = unit_vector(reflect_direction) + (fuzz * Vec3f::random_unit_vector());

        scattered = Ray(record.point, reflect_direction, ray_in.time());
        attenuation = albedo;
        return dot(scattered.direction(), record.normal) > 0;
    }

private:
    Color albedo;
    float fuzz;
};

#endif 
