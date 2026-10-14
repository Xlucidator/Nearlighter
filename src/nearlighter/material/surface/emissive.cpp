#include <nearlighter/material/surface/emissive.h>

#include <nearlighter/render/hit/surface_record.h>
#include <nearlighter/texture/solid_texture.h>

Emissive::Emissive(std::shared_ptr<Texture> tex)
    : texture(tex) {}

Emissive::Emissive(const Color& emit)
    : texture(std::make_shared<SolidTexture>(emit)) {}

ScatteringResponse Emissive::scattering(
    const HitRecord& record, TransportMode) const {
    record.surface();
    // A pure emitter ends scattering legitimately, unlike an unsupported model.
    return std::monostate{};
}

Color Emissive::emission(
    const HitRecord& hit, const Vec3f& outgoing) const {
    const auto& record = hit.surface();
    if (dot(record.outwardGeometricNormal(), outgoing) <= 0.0f) {
        return Color();
    }
    return texture->value(record.u, record.v,
                          record.point);
}
