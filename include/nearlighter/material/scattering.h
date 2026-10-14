#ifndef NEARLIGHTER_MATERIAL_SCATTERING_H
#define NEARLIGHTER_MATERIAL_SCATTERING_H

#include <nearlighter/base/color.h>
#include <cstdint>

/**
 * Quantity Transported by the Local Scattering Operator
 *
 * - Radiance evaluates incoming light; camera-path continuation uses this form.
 * - Importance transports sensor response; light subpaths use the adjoint form.
 * - Refraction needs different measure factors for these two operators.
 * - This choice does not select an importance-sampling strategy.
 */
enum class TransportMode { Radiance, Importance };

/**
 * Scattering Event Classification
 *
 * Reflection/transmission describe the surface side.
 * Diffuse/glossy/specular describe the angular response.
 * Specular denotes a delta event.
 */
enum class BxDFFlags : std::uint32_t {
    None = 0,
    Reflection = 1U << 0U,
    Transmission = 1U << 1U,
    Diffuse = 1U << 2U,
    Glossy = 1U << 3U,
    Specular = 1U << 4U,
};

constexpr BxDFFlags operator|(BxDFFlags left, BxDFFlags right) {
    return static_cast<BxDFFlags>(static_cast<std::uint32_t>(left) |
                                  static_cast<std::uint32_t>(right));
}

constexpr BxDFFlags operator&(BxDFFlags left, BxDFFlags right) {
    return static_cast<BxDFFlags>(static_cast<std::uint32_t>(left) &
                                  static_cast<std::uint32_t>(right));
}

constexpr bool hasAnyFlag(BxDFFlags value, BxDFFlags flags) {
    return (value & flags) != BxDFFlags::None;
}

/**
 * Evaluated Direction Sample
 *
 * - value contains the physical response, without a surface cosine.
 * - pdf is a solid-angle density or a discrete event probability.
 * - incoming is a unit continuation direction.
 * - The owning response defines the direction's coordinate space.
 * - Surface events use flags and eta; volume events retain their defaults.
 */
struct ScatteringSample {
    Color value;
    Vec3f incoming;
    float pdf = 0.0f;
    BxDFFlags flags = BxDFFlags::None;
    float eta = 1.0f;  // eta_after / eta_before; reflection: 1.
};

#endif  // NEARLIGHTER_MATERIAL_SCATTERING_H
