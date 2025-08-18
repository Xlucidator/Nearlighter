#ifndef DIFFUSE_LIGHT_H
#define DIFFUSE_LIGHT_H

#include "material.h"

class DiffuseLight : public Material {
public:
    DiffuseLight(shared_ptr<Texture> tex) : texture(tex) {}
    DiffuseLight(const Color& emit) : texture(make_shared<SolidTexture>(emit)) {}

    Color emitted(const Ray& ray_in, const HitRecord& record, float u, float v, const Point3f& p) const override {
        if (!record.front_face) return Color(0, 0, 0);
        return texture->value(u, v, p);
    }

private:
    shared_ptr<Texture> texture;
};

#endif // DIFFUSE_LIGHT_H
