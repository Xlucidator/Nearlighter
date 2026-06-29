#ifndef LAMBERTIAN_H
#define LAMBERTIAN_H

#include <nearlighter/material/material.h>

class Lambertian : public Material {
public:
    Lambertian(const Color& albedo);
    Lambertian(shared_ptr<Texture> tex);

    bool scatter(const Ray& ray_in, const HitRecord& record, ScatterRecord& s_record) const override;

    /* scatterPDF(w_o) = cos(theta_o) / pi
     */
    float getScatterPDF(const Ray& ray_in, const HitRecord& record, Ray& ray_scattered) const override;

private:
    shared_ptr<Texture> texture;
};

#endif // LAMBERTIAN_H
