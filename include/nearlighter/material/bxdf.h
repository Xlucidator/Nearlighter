#ifndef NEARLIGHTER_MATERIAL_BXDF_H
#define NEARLIGHTER_MATERIAL_BXDF_H

#include <nearlighter/base/color.h>
#include <nearlighter/math/vec2.h>

#include <cstdint>
#include <optional>

/** Transport Quantity for Refraction Weighting */
enum class TransportMode {
    Radiance,
    Importance,
};

/**
 * Scattering Event Classification
 *
 * Reflection/transmission describe the side; diffuse/glossy/specular describe
 * the directional response. Specular denotes a delta event in this renderer.
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
 * Local-Space Sample of One BxDF Component
 *
 * pdf is a solid-angle density or a delta-event probability.
 * The path weight is value * abs(cosine) / pdf.
 */
struct LocalBxDFSample {
    Color value;                          // f or delta coefficient; no cosine.
    Vec3f incoming;                       // Next ray: local unit direction.
    float pdf = 0.0f;                     // Excludes component selection.
    BxDFFlags flags = BxDFFlags::None;     // Actual sampled event.
    float eta = 1.0f;                     // eta_after / eta_before; reflection: 1.
};

/**
 * Local Surface-Scattering Component
 *
 * One component may describe reflection, transmission, or both.
 * It owns evaluated parameters and does not query textures or the scene.
 *
 * Direction convention (unit vectors in the shading frame, normal = +z):
 * - Both directions point away from the hit, possibly on opposite sides.
 * - On a camera path A -> P -> B, outgoing is P -> A, opposite the hit ray.
 * - incoming is P -> B; incident light physically travels along -incoming.
 */
class BxDF {
public:
    virtual ~BxDF() = default;

    /**
     * Continuous Scattering Value
     *
     * @param outgoing Local direction toward the previous camera-path vertex.
     * @param incoming Local candidate direction; may come from a light sample.
     * @param mode Transport quantity used for non-symmetric refraction factors.
     * @return f without the projected cosine; zero for delta-only models.
     */
    virtual Color evaluate(const Vec3f& outgoing, const Vec3f& incoming,
                           TransportMode mode) const = 0;

    /**
     * Continuous Sampling Density
     *
     * Uses the same local directions as evaluate(). The density is with
     * respect to solid angle; delta events contribute zero to this query.
     */
    virtual float PDF(const Vec3f& outgoing, const Vec3f& incoming,
                      TransportMode mode) const = 0;

    /**
     * Local Scattering Event
     *
     * @param outgoing Local direction toward the previous camera-path vertex.
     * @param direction_sample Independent uniform coordinates in [0, 1).
     * @param mode Transport quantity used for the returned event weight.
     * @return Sample whose incoming becomes the next continuation direction
     * after conversion to world space, or nullopt when no event is available.
     */
    virtual std::optional<LocalBxDFSample> sample(
        const Vec3f& outgoing, const Vec2f& direction_sample,
        TransportMode mode) const = 0;

    /** Union of All Event Categories Supported by This Component */
    virtual BxDFFlags flags() const = 0;
};

#endif  // NEARLIGHTER_MATERIAL_BXDF_H
