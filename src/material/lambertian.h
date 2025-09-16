#ifndef LAMBERTIAN_H
#define LAMBERTIAN_H

#include "material.h"

class Lambertian : public Material {
public:
    Lambertian(const Color& albedo) : texture(make_shared<SolidTexture>(albedo)) {}
    Lambertian(shared_ptr<Texture> tex) : texture(tex) {}

    bool scatter(
        const Ray& ray_in, const HitRecord& record, ScatterRecord& s_record
    ) const override {
        s_record.attenuation = texture->value(record.u, record.v, record.point);
        s_record.pdf = make_shared<CosineHemispherePDF>(record.normal);
        s_record.should_skip = false;
        return true;
    }

    /* scatterPDF(w_o) = cos(theta_o) / pi
     */
    float getScatterPDF(const Ray& ray_in, const HitRecord& record, Ray& ray_scattered) const override {
        float cos_theta_o = dot(record.normal, unit_vector(ray_scattered.direction()));
        return cos_theta_o > 0.0f ? cos_theta_o / pi : 0.0f;
    }

private:
    shared_ptr<Texture> texture;
};

#endif // LAMBERTIAN_H
