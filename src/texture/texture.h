#ifndef TEXTURE_H
#define TEXTURE_H

#include "common.h"

class Texture {
public:
    virtual Color value(float u, float v, const Point3f& p) const = 0;
};

/* Derived Material Implement */

class SolidTexture : public Texture {
public:
    SolidTexture(const Color& albedo) : albedo(albedo) {}
    SolidTexture(float red, float green, float blue) : SolidTexture(Color(red, green, blue)) {}

    Color value(float u, float v, const Point3f& p) const override { return albedo; }

  private:
    Color albedo;
};

#include "checker.h"
#include "image.h"
#include "noise.h"

#endif // TEXTURE_H