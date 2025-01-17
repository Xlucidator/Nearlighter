#ifndef METAL_H
#define METAL_H

#include "material.h"

class Metal : public Material {
public:
    Metal(const Color& albedo) : albedo(albedo) {}

    bool scatter(const Ray& ray_in, const HitRecord& record, Color& attenuation, Ray& scattered) const override {
        Vec3f reflect_direction = reflect(ray_in.direction(), record.normal);

        scattered = Ray(record.point, reflect_direction);
        attenuation = albedo;
        return true;
    }

private:
    Color albedo;
};

#endif 
