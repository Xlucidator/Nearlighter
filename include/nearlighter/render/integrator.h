#ifndef NEARLIGHTER_RENDER_INTEGRATOR_H
#define NEARLIGHTER_RENDER_INTEGRATOR_H

#include <nearlighter/render/film.h>
#include <nearlighter/render/render_settings.h>

#include <chrono>
#include <cstdint>
#include <functional>

class RenderContext;
class Ray;
class Sampler;
class Scene;

/** Row-level integration state. */
struct RenderProgress {
    int completed_rows = 0;
    int total_rows = 0;
    std::chrono::duration<double> integration_time{};
};

/**
 * Receives row-level progress and a read-only view of the partial Film.
 *
 * Rows before completed_rows contain their final sample counts.
 * The Film reference is valid only during this callback.
 */
using RenderProgressCallback =
    std::function<void(const RenderProgress&, const Film&)>;

/** Low-overhead aggregate diagnostics produced by one integration pass. */
struct IntegratorCounters {
    std::uint64_t sample_count = 0;
    std::uint64_t camera_rays = 0;
    std::uint64_t continuation_rays = 0;
    std::uint64_t shadow_rays = 0;
    std::uint64_t surface_interactions = 0;
    std::uint64_t medium_interactions = 0;
    std::uint64_t path_length_sum = 0;
    std::uint64_t max_depth_terminations = 0;
    std::uint64_t russian_roulette_terminations = 0;
    std::uint64_t invalid_pdf_terminations = 0;
    std::uint64_t invalid_contributions = 0;
};

/** Integration result excluding wall time measured by Renderer. */
struct IntegrationResult {
    IntegratorCounters counters;
    std::chrono::steady_clock::duration callback_time{};
};

/** Complete render algorithm that owns primary-path scheduling. */
class Integrator {
public:
    virtual ~Integrator() = default;

    /** Integrates every configured primary sample into the supplied Film. */
    virtual IntegrationResult render(
        const RenderContext& context, Film& film,
        RenderProgressCallback progress_callback) const = 0;
};

/** Iterative Surface and Volume Path Tracer */
class PathIntegrator final : public Integrator {
public:
    explicit PathIntegrator(PathIntegratorOptions options);

    /** Checks Scene Content against the Requested Sampling Mode before Tracing */
    static void validateScene(const Scene& scene, const PathIntegratorOptions& options);

    IntegrationResult render(
        const RenderContext& context, Film& film,
        RenderProgressCallback progress_callback) const override;

private:
    FilmSample trace(const Ray& camera_ray, const RenderContext& context,
                     Sampler& sampler, IntegratorCounters& counters,
                     std::uint64_t& path_length) const;

    PathIntegratorOptions options_;
};

#endif  // NEARLIGHTER_RENDER_INTEGRATOR_H
