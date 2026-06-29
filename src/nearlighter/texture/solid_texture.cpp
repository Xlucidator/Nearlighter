#include <nearlighter/texture/solid_texture.h>

SolidTexture::SolidTexture(const Color& albedo)
    : albedo(albedo) {}

SolidTexture::SolidTexture(float red, float green, float blue)
    : SolidTexture(Color(red, green, blue)) {}

Color SolidTexture::value(float, float, const Point3f&) const {
    return albedo;
}
