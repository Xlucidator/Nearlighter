#include <nearlighter/material/medium/homogeneous.h>
#include <nearlighter/material/medium/isotropic.h>
#include <nearlighter/render/hit/medium_record.h>
#include <nearlighter/texture/solid_texture.h>

#include <cmath>
#include <stdexcept>
#include <utility>

namespace {
void validateAlbedo(const Color& albedo) {
    for (std::size_t axis = 0; axis < 3; ++axis) {
        if (!(albedo[axis] >= 0.0f && albedo[axis] <= 1.0f)) {
            throw std::invalid_argument("Medium albedo must lie in [0, 1]");
        }
    }
}
}  // namespace

Homogeneous::Homogeneous(float extinction, Color albedo,
                         std::shared_ptr<const PhaseFunction> phase)
    : Homogeneous(extinction, std::make_shared<SolidTexture>(albedo),
                  std::move(phase)) {
    validateAlbedo(albedo);
}

Homogeneous::Homogeneous(float extinction, std::shared_ptr<const Texture> albedo,
                         std::shared_ptr<const PhaseFunction> phase)
    : extinction_(extinction), albedo_(std::move(albedo)), phase_(std::move(phase)) {
    if (!std::isfinite(extinction_) || extinction_ <= 0.0f || !albedo_) {
        throw std::invalid_argument("Homogeneous requires positive extinction and an albedo texture");
    }
    if (!phase_) {
        static const auto isotropic = std::make_shared<const Isotropic>();
        phase_ = isotropic;
    }
}

ScatteringResponse Homogeneous::scattering(
    const HitRecord& hit, TransportMode) const {
    const auto& record = hit.medium();
    // Volume textures use spatial coordinates; there is no surface UV mapping.
    const Color albedo = albedo_->value(0.0f, 0.0f, record.point);
    return PhaseResponse(albedo, *phase_);
}
