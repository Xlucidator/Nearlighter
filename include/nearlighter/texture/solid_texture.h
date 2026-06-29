#ifndef SOLID_TEXTURE_H
#define SOLID_TEXTURE_H

#include <nearlighter/texture/texture.h>

class SolidTexture : public Texture {
public:
    SolidTexture(const Color& albedo);
    SolidTexture(float red, float green, float blue);

    Color value(float u, float v, const Point3f& p) const override;

private:
    Color albedo;
};

#endif // SOLID_TEXTURE_H
