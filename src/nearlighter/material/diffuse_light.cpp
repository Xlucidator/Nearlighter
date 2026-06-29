#include <nearlighter/material/diffuse_light.h>

#include <nearlighter/texture/solid_texture.h>

DiffuseLight::DiffuseLight(shared_ptr<Texture> tex)
    : texture(tex) {}

DiffuseLight::DiffuseLight(const Color& emit)
    : texture(make_shared<SolidTexture>(emit)) {}

Color DiffuseLight::emitted(const Ray&, const HitRecord& record, float u, float v, const Point3f& p) const {
    if (!record.front_face) return Color(0, 0, 0);
    return texture->value(u, v, p);
}
