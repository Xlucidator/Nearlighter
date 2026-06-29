#ifndef DIFFUSE_LIGHT_H
#define DIFFUSE_LIGHT_H

#include <nearlighter/material/material.h>

class DiffuseLight : public Material {
public:
    DiffuseLight(shared_ptr<Texture> tex);
    DiffuseLight(const Color& emit);

    Color emitted(const Ray& ray_in, const HitRecord& record, float u, float v, const Point3f& p) const override;

private:
    shared_ptr<Texture> texture;
};

#endif // DIFFUSE_LIGHT_H
