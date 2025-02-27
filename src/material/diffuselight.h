#ifndef DIFFUSE_LIGHT_H
#define DIFFUSE_LIGHT_H

#include "material.h"

class DiffuseLight : public Material {
public:
    DiffuseLight(shared_ptr<Texture> tex) : texture(tex) {}
    DiffuseLight(const Color& emit) : texture(make_shared<SolidTexture>(emit)) {}

    Color emitted(float u, float v, const Point3f& p) const override {
        return texture->value(u, v, p);
    }

private:
    shared_ptr<Texture> texture;
};

#endif // DIFFUSE_LIGHT_H
