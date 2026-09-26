#include <nearlighter/material/emissive.h>

#include <nearlighter/scene/intersectable.h>
#include <nearlighter/scene/surface_interaction.h>
#include <nearlighter/texture/solid_texture.h>

Emissive::Emissive(std::shared_ptr<Texture> tex)
    : texture(tex) {}

Emissive::Emissive(const Color& emit)
    : texture(std::make_shared<SolidTexture>(emit)) {}

std::optional<BSDF> Emissive::computeBSDF(
    const SurfaceInteraction&, TransportMode) const {
    // A pure emitter ends scattering legitimately, unlike an unsupported model.
    return std::nullopt;
}

Color Emissive::evaluateEmission(
    const SurfaceInteraction& interaction, const Vec3f& outgoing) const {
    if (dot(interaction.outwardGeometricNormal(), outgoing) <= 0.0f) {
        return Color();
    }
    return texture->value(interaction.u(), interaction.v(),
                          interaction.point());
}

Color Emissive::emitted(const Ray&, const HitRecord& record,
                       float u, float v, const Point3f& p) const {
    if (!record.front_face) return Color(0, 0, 0);
    return texture->value(u, v, p);
}
