#ifndef LAMBERTIAN_H
#define LAMBERTIAN_H

#include <nearlighter/material/material.h>

#include <memory>

class Lambertian : public Material {
public:
    Lambertian(const Color& albedo);
    Lambertian(std::shared_ptr<Texture> tex);

    bool scatter(const Ray& ray_in, const HitRecord& record,
                 ScatterRecord& s_record, Sampler& sampler) const override;

    /** Evaluates the cosine-weighted material sampling density. */
    float getScatterPDFValue(const Ray& ray_in, const HitRecord& record,
                             const Ray& ray_scattered) const override;

private:
    std::shared_ptr<Texture> texture;
};

#endif // LAMBERTIAN_H
