#ifndef ISOTROPIC_H
#define ISOTROPIC_H

#include "material.h"

class Isotropic : public Material {
public:
    Isotropic(shared_ptr<Texture> tex) : texture(tex) {}
    Isotropic(const Color& albedo) : texture(make_shared<SolidTexture>(albedo)) {}

    bool scatter(const Ray& ray_in, const HitRecord& record, Color& attenuation, Ray& scattered) const override {
        scattered = Ray(record.point, Vec3f::random_unit_vector(), ray_in.time());
        attenuation = texture->value(record.u, record.v, record.point);
        // std::cout << "attenuation = " << attenuation << std::endl;
        return true;
    }

private:
    shared_ptr<Texture> texture;
};


#endif // ISOTROPIC_H
