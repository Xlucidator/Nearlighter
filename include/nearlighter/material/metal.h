#ifndef METAL_H
#define METAL_H

#include <nearlighter/material/material.h>

class Metal : public Material {
public:
    Metal(const Color& albedo, float fuzz = 0.0f);

    bool scatter(const Ray& ray_in, const HitRecord& record,
                 ScatterRecord& s_record, Sampler& sampler) const override;

private:
    Color albedo;
    float fuzz;
};

#endif 
