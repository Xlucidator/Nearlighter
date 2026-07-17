#ifndef ISOTROPIC_H
#define ISOTROPIC_H

#include <nearlighter/material/material.h>

class Isotropic : public Material {
public:
    Isotropic(shared_ptr<Texture> tex);
    Isotropic(const Color& albedo);

    bool scatter(const Ray& ray_in, const HitRecord& record,
                 ScatterRecord& s_record, Sampler& sampler) const override;

    float getScatterPDF(const Ray& ray_in, const HitRecord& record,
                        const Ray& ray_scattered) const override;

private:
    shared_ptr<Texture> texture;
};


#endif // ISOTROPIC_H
