#ifndef ISOTROPIC_H
#define ISOTROPIC_H

#include "material.h"

class Isotropic : public Material {
public:
    Isotropic(shared_ptr<Texture> tex) : texture(tex) {}
    Isotropic(const Color& albedo) : texture(make_shared<SolidTexture>(albedo)) {}

    bool scatter(
        const Ray& ray_in, const HitRecord& record, Color& attenuation, Ray& scattered,
        float& pdf
    ) const override {
        scattered = Ray(record.point, Vec3f::random_unit_vector(), ray_in.time());
        attenuation = texture->value(record.u, record.v, record.point);
        // std::cout << "attenuation = " << attenuation << std::endl;
        pdf = 1 / (4 * pi);
        return true;
    }

    float getScatterPDF(const Ray& ray_in, const HitRecord& record, Ray& ray_scattered) const override {
        return 1 / (4 * pi);
    }

private:
    shared_ptr<Texture> texture;
};


#endif // ISOTROPIC_H
