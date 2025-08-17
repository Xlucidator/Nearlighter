#ifndef LAMBERTIAN_H
#define LAMBERTIAN_H

#include "material.h"

class Lambertian : public Material {
public:
    Lambertian(const Color& albedo) : texture(make_shared<SolidTexture>(albedo)) {}
    Lambertian(shared_ptr<Texture> tex) : texture(tex) {}

    bool scatter(
        const Ray& ray_in, const HitRecord& record, Color& attenuation, Ray& scattered, 
        float& pdf
    ) const override {
        ONB normal_basis = ONB(record.normal);
        Vec3f scatter_direction = normal_basis.transform(random_cosine_unit_on_hemisphere());

        scattered = Ray(record.point, scatter_direction, ray_in.time());
        attenuation = texture->value(record.u, record.v, record.point);
        pdf = dot(normal_basis.w(), scattered.direction()) / pi;
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
