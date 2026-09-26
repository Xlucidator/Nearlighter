#ifndef NEARLIGHTER_RENDER_INTEGRATOR_H
#define NEARLIGHTER_RENDER_INTEGRATOR_H

#include <nearlighter/render/film.h>
#include <nearlighter/render/render_settings.h>
#include <nearlighter/render/render_context.h>

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

class Intersectable;
class EnvironmentLight;
class Primitive;
class Ray;
class Sampler;
class Scene;

/** Selects the estimator used for non-delta direct illumination. */
enum class DirectLightingMode {
    BSDFOnly,
    LightOnly,
    MIS,
};

/** Selects a complete render algorithm without changing Scene state. */
enum class IntegratorKind {
    Legacy,
    Path,
};

/** Algorithm parameters owned by the iterative surface path integrator. */
struct PathIntegratorOptions {
    // Zero inherits the transitional max_depth value from RenderSettings.
    int max_depth = 0;

    // Zero disables Russian roulette; positive values count scattering events.
    int russian_roulette_start_depth = 0;
    DirectLightingMode direct_lighting = DirectLightingMode::MIS;
};

/** Selects the integrator and in-memory outputs for one Renderer. */
struct RenderOptions {
    IntegratorKind integrator = IntegratorKind::Legacy;
    PathIntegratorOptions path;
    FilmAOV aovs = FilmAOV::Beauty;
};

/** Row-level integration state. */
struct RenderProgress {
    int completed_rows = 0;
    int total_rows = 0;
    std::chrono::duration<double> integration_time{};
};

/**
 * Receives row-level progress and a read-only view of the partial Film.
 *
 * Rows before completed_rows contain their final sample counts. The Film
 * reference is valid only for the duration of the callback invocation.
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

/** Read-only compatibility implementation of the former recursive renderer. */
class LegacyPathIntegrator final : public Integrator {
public:
    explicit LegacyPathIntegrator(int max_depth) : max_depth_(max_depth) {}

    IntegrationResult render(
        const RenderContext& context, Film& film,
        RenderProgressCallback progress_callback) const override;

private:
    Color trace(
        const Ray& ray, int depth, const Intersectable& world,
        const std::vector<std::shared_ptr<const Primitive>>& sampling_targets,
        const EnvironmentLight* environment, Sampler& sampler,
        IntegratorCounters& counters, std::uint64_t& path_length) const;

    int max_depth_ = 0;
};

/** Iterative surface path tracer with explicit NEE and configurable MIS. */
class PathIntegrator final : public Integrator {
public:
    explicit PathIntegrator(PathIntegratorOptions options)
        : options_(options) {}

    /** Checks surface/material capabilities before any camera sample is taken. */
    static void validateScene(const Scene& scene);

    IntegrationResult render(
        const RenderContext& context, Film& film,
        RenderProgressCallback progress_callback) const override;

private:
    FilmSample trace(const Ray& camera_ray, const RenderContext& context,
                     Sampler& sampler, IntegratorCounters& counters,
                     std::uint64_t& path_length) const;

    PathIntegratorOptions options_;
};

/** Creates the selected concrete Integrator after resolving path defaults. */
std::unique_ptr<Integrator> createIntegrator(
    const RenderSettings& settings, const RenderOptions& options);

#endif  // NEARLIGHTER_RENDER_INTEGRATOR_H
