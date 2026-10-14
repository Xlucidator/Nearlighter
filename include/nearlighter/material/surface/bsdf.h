#ifndef NEARLIGHTER_MATERIAL_SURFACE_BSDF_H
#define NEARLIGHTER_MATERIAL_SURFACE_BSDF_H

#include <nearlighter/geometry/shading_frame.h>
#include <nearlighter/material/surface/bxdf.h>

#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <utility>

/**
 * Per-Hit Surface-Scattering Aggregate
 *
 * Owns its frame and evaluated components, without retaining the record.
 * Components add physical responses; uniform selection only mixes proposals.
 *
 * All public directions are world-space and point away from the hit:
 * - outgoing points to the previous camera-path vertex, opposite the hit ray.
 * - incoming points toward a candidate light or the next continuation vertex.
 * Incident light propagates along -incoming, opposite camera tracing order.
 *
 * Non-unit directions are normalized.
 * Invalid directions and empty BSDFs yield zero evaluations or no sample.
 */
class BSDF final : public SampleableFunction<Vec3f, Vec3f, Color, std::optional<ScatteringSample>> {
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

    /** Union of Component Event Categories */
    BxDFFlags flags() const;

    bool empty() const { return component_count_ == 0; }
    std::size_t componentCount() const { return component_count_; }

    /**
     * Aggregate Continuous Scattering Value
     *
     * @param outgoing World direction toward the previous camera-path vertex.
     * @param incoming World candidate direction, including light-sampled directions.
     * @return Sum of component values, without cosine; delta components add zero.
     */
    Color evaluate(const Vec3f& outgoing, const Vec3f& incoming) const override;

    /**
     * Mixture Solid-Angle Density
     *
     * Uses the same world directions as evaluate().
     * Uniform selection gives sum(component PDFs) / componentCount().
     * Delta components contribute zero continuous density.
     */
    float PDF(const Vec3f& outgoing, const Vec3f& incoming) const override;

    /**
     * World-Space Scattering Event
     *
     * Continuous samples contain aggregate f and mixture PDF.
     * Delta samples retain the selected coefficient and include its selection PMF.
     *
     * @param outgoing World direction opposite the ray that reached this hit.
     * @return Sample whose incoming is the next camera-path ray direction.
     */
    std::optional<ScatteringSample> sample(
        const Vec3f& outgoing, Sampler& sampler) const override;

private:
    ShadingFrame frame_;
    // Every counted slot is non-null, including after failed insertion or move.
    std::array<std::unique_ptr<BxDF>, kMaximumComponents> components_{};
    std::size_t component_count_ = 0;
};

#endif  // NEARLIGHTER_MATERIAL_SURFACE_BSDF_H
