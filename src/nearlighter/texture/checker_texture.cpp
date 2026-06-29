#include <nearlighter/texture/checker_texture.h>

#include <nearlighter/texture/solid_texture.h>

#include <cmath>

CheckerTexture::CheckerTexture(float scale, shared_ptr<Texture> even, shared_ptr<Texture> odd)
    : inv_scale(1.0f / scale), even(even), odd(odd) {}

CheckerTexture::CheckerTexture(float scale, const Color& even, const Color& odd)
    : inv_scale(1.0f / scale),
      even(make_shared<SolidTexture>(even)),
      odd(make_shared<SolidTexture>(odd)) {}

Color CheckerTexture::value(float u, float v, const Point3f& p) const {
    int x = int(std::floor(inv_scale * p.x()));
    int y = int(std::floor(inv_scale * p.y()));
    int z = int(std::floor(inv_scale * p.z()));

    return ((x + y + z) & 1) == 0 ? even->value(u, v, p) : odd->value(u, v, p);
}
