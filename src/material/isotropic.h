#ifndef ISOTROPIC_H
#define ISOTROPIC_H

#include "material.h"

class Isotropic : public Material {
public:
    Isotropic(shared_ptr<Texture> tex) : texture(tex) {}
    Isotropic(const Color& albedo) : texture(make_shared<SolidTexture>(albedo)) {}

    bool scatter(
        const Ray& ray_in, const HitRecord& record, ScatterRecord& s_record
    ) const override {
        s_record.attenuation = texture->value(record.u, record.v, record.point);
        s_record.pdf = make_shared<SpherePDF>();
        s_record.should_skip = false;
        return true;
    }

    float getScatterPDF(const Ray& ray_in, const HitRecord& record, Ray& ray_scattered) const override {
        return 1 / (4 * pi);
    }

private:
    shared_ptr<Texture> texture;
};


#endif // ISOTROPIC_H
