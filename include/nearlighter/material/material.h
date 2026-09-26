#ifndef MATERIAL_H
#define MATERIAL_H

#include <nearlighter/base/color.h>
#include <nearlighter/base/ray.h>
#include <nearlighter/geometry/shading_frame.h>
#include <nearlighter/material/bxdf.h>

#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <utility>

class PDF;
class Sampler;
class SurfaceInteraction;
struct HitRecord;

// ==================================================
// Evaluated Surface Scattering
// ==================================================

/**
 * World-Space Sample of the Complete BSDF
 *
 * pdf is a solid-angle density or a delta-event probability.
 * The path weight is value * abs(cosine) / pdf.
 */
struct BSDFSample {
    Color value;                          // Summed f or delta coefficient.
    Vec3f incoming;                       // Next ray: world unit direction.
    float pdf = 0.0f;                     // Includes component selection.
    BxDFFlags flags = BxDFFlags::None;     // Actual sampled event.
    float eta = 1.0f;                     // eta_after / eta_before; reflection: 1.
};

/**
 * Per-Hit Surface-Scattering Aggregate
 *
 * Owns its frame and evaluated components, without retaining the interaction.
 * Components add physical responses; uniform selection only mixes proposals.
 *
 * All public directions are world-space and point away from the hit:
 * - outgoing points to the previous camera-path vertex, opposite the hit ray.
 * - incoming points toward a candidate light or the next continuation vertex.
 * Incident light propagates along -incoming, opposite camera tracing order.
 *
 * Non-unit directions are normalized. Invalid or near-zero directions produce
 * zero evaluations or no sample. An empty BSDF has the same zero response.
 */
class BSDF {
public:
    static constexpr std::size_t kMaximumComponents = 4;

    explicit BSDF(ShadingFrame frame) : frame_(std::move(frame)) {}

    BSDF(const BSDF&) = delete;
    BSDF& operator=(const BSDF&) = delete;

    /** @name Exclusive Ownership Transfer
     * The source becomes empty; self-assignment leaves the object unchanged.
     * @{ */
    BSDF(BSDF&& other) noexcept;
    BSDF& operator=(BSDF&& other) noexcept;
    /** @} */

    /**
     * Exclusive Component Insertion
     *
     * A failed insertion leaves the existing components unchanged.
     * @throws std::invalid_argument for null.
     * @throws std::length_error beyond the fixed capacity.
     */
    void add(std::unique_ptr<BxDF> component);

    bool empty() const { return component_count_ == 0; }
    std::size_t componentCount() const { return component_count_; }

    /**
     * Aggregate Continuous Scattering Value
     *
     * @param outgoing World direction toward the previous camera-path vertex.
     * @param incoming World candidate direction, including light-sampled directions.
     * @param mode Transport quantity used by the components.
     * @return Sum of component values, without cosine; delta components add zero.
     */
    Color evaluate(const Vec3f& outgoing, const Vec3f& incoming,
                   TransportMode mode) const;

    /**
     * Mixture Solid-Angle Density
     *
     * Uses the same world directions as evaluate(). Uniform component selection
     * gives sum(component PDFs) / componentCount(); delta components add zero.
     */
    float PDF(const Vec3f& outgoing, const Vec3f& incoming,
              TransportMode mode) const;

    /**
     * World-Space Scattering Event
     *
     * Continuous samples contain aggregate f and mixture PDF. Delta samples
     * retain the selected coefficient and include selection in their probability.
     *
     * @param outgoing World direction opposite the ray that reached this hit.
     * @param component_sample Uniform component-selection coordinate in [0, 1).
     * @param direction_sample Independent uniform coordinates in [0, 1).
     * @param mode Transport quantity used for the returned event weight.
     * @return Sample whose incoming is the next camera-path ray direction.
     * @throws std::invalid_argument for invalid random coordinates when the
     * BSDF is nonempty and outgoing is valid.
     */
    std::optional<BSDFSample> sample(
        const Vec3f& outgoing, float component_sample,
        const Vec2f& direction_sample, TransportMode mode) const;

private:
    ShadingFrame frame_;
    // Every counted slot is non-null, including after failed insertion or move.
    std::array<std::unique_ptr<BxDF>, kMaximumComponents> components_{};
    std::size_t component_count_ = 0;
};

// ==================================================
// Material Entry and Legacy Compatibility
// ==================================================

/** Legacy Scatter Result; Not Used by the BSDF Path */
struct ScatterRecord {
    Color attenuation;
    std::shared_ptr<PDF> sampling_pdf;
    bool should_skip;
    Ray skip_ray;
};

/**
 * Shared Material Parameters and Per-Hit Evaluation
 *
 * Stores textures and model configuration; computeBSDF() resolves them into
 * independent scattering objects without mutating shared material state.
 * The legacy hooks also serve Isotropic until volume transport is migrated.
 */
class Material {
public:
    virtual ~Material() = default;

    /**
     * Evaluated Scattering at One Surface Hit
     *
     * @return An independently owned, nonempty BSDF for scattering, or nullopt
     * for a supported surface with no scattering (such as a pure emitter).
     * @throws std::runtime_error when the new path has no implementation.
     */
    virtual std::optional<BSDF> computeBSDF(
        const SurfaceInteraction& interaction, TransportMode mode) const;

    /** Migration-Time Capability Check Before Entering the Path Integrator */
    virtual bool supportsPathIntegrator() const { return false; }

    /** Potential Emission for Automatic Light Discovery; Not a Per-Hit Test */
    virtual bool isEmissive() const { return false; }

    /**
     * Emitted Radiance Along an Outward World Direction
     *
     * outgoing points from the hit toward the receiver, or the previous
     * camera-path vertex; it is opposite the arriving camera ray direction.
     */
    virtual Color evaluateEmission(
        [[maybe_unused]] const SurfaceInteraction& interaction,
        [[maybe_unused]] const Vec3f& outgoing) const {
        return Color();
    }

    // ==================================================
    // Legacy Integrator Hooks
    // ==================================================

    /** Legacy Emission Toward the Ray's Previous Vertex; Black by Default */
    virtual Color emitted(
        [[maybe_unused]] const Ray& ray_in,
        [[maybe_unused]] const HitRecord& record,
        [[maybe_unused]] float u,
        [[maybe_unused]] float v,
        [[maybe_unused]] const Point3f& p
    ) const { return Color(0, 0, 0); }

    /**
     * Legacy Continuation Proposal
     *
     * ray_in travels toward the hit. On success, s_record contains attenuation
     * and either a directional PDF or a directly selected skip_ray.
     */
    virtual bool scatter(
        [[maybe_unused]] const Ray& ray_in,
        [[maybe_unused]] const HitRecord& record,
        [[maybe_unused]] ScatterRecord& s_record,
        [[maybe_unused]] Sampler& sampler
    ) const { return false; }

    /** Legacy Scattering Density for a Ray Traveling Away from the Hit */
    virtual float getScatterPDFValue(
        [[maybe_unused]] const Ray& ray_in,
        [[maybe_unused]] const HitRecord& record,
        [[maybe_unused]] const Ray& ray_scattered
    ) const { return 0.0f; }
};

#endif  // MATERIAL_H
