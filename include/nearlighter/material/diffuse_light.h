#ifndef DIFFUSE_LIGHT_H
#define DIFFUSE_LIGHT_H

#include <nearlighter/material/material.h>

#include <memory>

class DiffuseLight : public Material {
public:
    DiffuseLight(std::shared_ptr<Texture> tex);
    DiffuseLight(const Color& emit);

    Color emitted(const Ray& ray_in, const HitRecord& record, float u, float v, const Point3f& p) const override;

private:
    std::shared_ptr<Texture> texture;
};

#endif // DIFFUSE_LIGHT_H
